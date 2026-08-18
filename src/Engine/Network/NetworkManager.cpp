#include "Engine/Network/NetworkManager.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkPath.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeType.h"
#include "Engine/Network/UpdateLock.h"
#include "Engine/UndoRedo/ChangeConnectionCommand.h"
#include "Engine/UndoRedo/CreateNodeCommand.h"
#include "Engine/UndoRedo/DeleteNodeCommand.h"
#include "Engine/UndoRedo/MoveNodeCommand.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace enzo {

nt::NodeId nt::NetworkManager::createNode(
    const nt::NodeType& nodeType,
    const Path& parent,
    const std::string& name,
    Vector2 position
)
{
    // An unnamed node is numbered from its type name, so the first grid becomes "grid1"
    Path path = parent.append(Path(name.empty() ? nodeType.internalName + "1" : name));
    while (getNodeAtPath(path))
        path = path.increment();

    NodeId nodeId = network_.reserveNodeId();
    createNodeWithId(nodeId, nodeType, path, position);

    return nodeId;
}

void nt::NetworkManager::moveNode(NodeId nodeId, Vector2 newPos, bool skipUndo)
{
    Vector2 oldPos = getNode(nodeId).getPosition();
    getNode(nodeId).setPosition(newPos);

    if (!skipUndo)
    {
        auto cmd = std::make_unique<MoveNodeCommand>(nodeId, oldPos, newPos);
        undoStack_.push(std::move(cmd));
    }

    nodePositionChanged(nodeId, newPos);
}

void nt::NetworkManager::deleteNode(NodeId nodeId)
{
    if (!isValidNode(nodeId)) return;

    auto updateLock = lockUpdates();

    // Group everything this delete touches into one atomic undo unit
    UndoTransaction transaction(undoStack_);

    // A node holding a scope takes its contents with it. Children are recorded first
    // so undo brings the container back before the nodes that live inside it.
    for (NodeId child : getChildNodeIds(getNode(nodeId).getPath()))
        deleteNode(child);

    // Disconnect first so the reconnects replay after the node is restored on undo
    disconnectNode(nodeId);

    auto cmd = std::make_unique<DeleteNodeCommand>(nodeId);
    undoStack_.push(std::move(cmd));

    // Release the display, primary, and selection state pointing at this node
    if (displayNode_ == nodeId) clearDisplayFlag();
    if (primaryNode_ == nodeId) clearPrimaryNode();
    setSelectedNode(nodeId, false, true);

    // Signal before erasing so listeners can still query the node
    nodeRemoved(nodeId);

    network_.deleteNode(nodeId);
}

void nt::NetworkManager::createNodeWithId(
    NodeId nodeId,
    const nt::NodeType& nodeType,
    const Path& path,
    Vector2 position
)
{
    const Path scope = path.getParent();
    if (!getScope(scope))
        throw std::out_of_range("no scope at " + scope.getString() + " to create a node in\n");

    Node& node = network_.createNode(nodeId, nodeType, path);
    node.setPosition(position);
    node.nodeDirtied.connect([this](nt::NodeId nodeId, bool dirtyDependents) {
        onNodeDirtied(nodeId, dirtyDependents);
    });

    auto cmd = std::make_unique<CreateNodeCommand>(nodeId);
    undoStack_.push(std::move(cmd));

    nodeCreated(nodeId);
}

void nt::NetworkManager::disconnectNode(NodeId nodeId)
{
    if (!isValidNode(nodeId)) return;

    // getInputs and getOutputs return copies, so disconnecting while iterating is safe
    for (const nt::Connection& connection : graph().getInputs(nodeId))
    {
        disconnectNodes(connection);
    }

    for (const nt::Connection& connection : graph().getOutputs(nodeId))
    {
        disconnectNodes(connection);
    }
}

nt::NetworkManager& nt::NetworkManager::getInstance()
{
    static nt::NetworkManager instance;
    return instance;
}

nt::Node& nt::NetworkManager::getNode(nt::NodeId nodeId) { return network_.getNode(nodeId); }

nt::Node* nt::NetworkManager::findNode(const NetworkPath& path, NodeId fromNode)
{
    NetworkPath nodePath = path.getNode();

    // An empty node path names the node the lookup starts from.
    if (nodePath.isEmpty()) return isValidNode(fromNode) ? &getNode(fromNode) : nullptr;

    // A relative path is read from the scope holding the asking node
    Path anchor = isValidNode(fromNode) ? getNode(fromNode).getPath().getParent() : Path("/");

    return getNodeAtPath(nodePath.makeAbsoluteFrom(anchor));
}

std::weak_ptr<prm::NodeParameter>
nt::NetworkManager::findParameter(const NetworkPath& path, NodeId fromNode)
{
    if (!path.hasParameter()) return {};

    Node* node = findNode(path, fromNode);
    if (!node) return {};

    return node->getParameter(path.getParameter());
}

bool nt::NetworkManager::isValidNode(nt::NodeId nodeId) { return network_.isValidNode(nodeId); }

void nt::NetworkManager::setDisplayNode(NodeId nodeId)
{
    displayNode_ = nodeId;

    cook(nodeId);

    nt::Node& displayNode = getNode(nodeId);
    displayGeoChanged(displayNode.getOutputPacket(0));
    displayNodeChanged(nodeId);
}

void nt::NetworkManager::clearDisplayFlag()
{
    displayNode_.reset();
    displayGeoChanged(std::make_shared<const NodePacket>());
    displayNodeChanged(std::nullopt);
}

std::optional<nt::NodeId> nt::NetworkManager::getPrimaryNode() { return primaryNode_; }

void nt::NetworkManager::setPrimaryNode(NodeId nodeId)
{
    primaryNode_ = nodeId;

    cook(nodeId);

    nt::Node& primaryNode = getNode(nodeId);
    primaryGeoChanged(primaryNode.getOutputPacket(0));
    primaryNodeChanged(nodeId);
}

void nt::NetworkManager::clearPrimaryNode()
{
    primaryNode_.reset();
    primaryGeoChanged(std::make_shared<const NodePacket>());
    primaryNodeChanged(std::nullopt);
}

void nt::NetworkManager::setSelectedNode(NodeId nodeId, bool selected, bool add)
{
    if (add)
    {
        auto idIter = std::find(selectedNodes_.begin(), selectedNodes_.end(), nodeId);
        if (selected)
        {
            // skip if value is already in selected nodes
            if (idIter != selectedNodes_.end()) return;
            selectedNodes_.push_back(nodeId);
            cook(nodeId);
        }
        else
        {
            // skip if value is not in selected nodes
            if (idIter == selectedNodes_.end()) return;
            selectedNodes_.erase(idIter);
        }
    }
    else
    {
        if (selected)
        {
            selectedNodes_.clear();
            selectedNodes_.push_back(nodeId);
            cook(nodeId);
        }
        else
        {
            selectedNodes_.clear();
        }
    }
    selectedNodesChanged(selectedNodes_);
}

nt::UpdateLock nt::NetworkManager::lockUpdates() { return UpdateLock(); }

void nt::NetworkManager::update()
{
    // cook display node
    if (getDisplayNode().has_value())
    {

        const NodeId displayNodeId = getDisplayNode().value();
        cook(displayNodeId);

        auto& displayNode = getNode(displayNodeId);
        displayGeoChanged(displayNode.getOutputPacket(0));
    }

    // cook primary node and notify the geometry pane
    if (getPrimaryNode().has_value())
    {
        const NodeId primaryNodeId = getPrimaryNode().value();
        cook(primaryNodeId);

        auto& primaryNode = getNode(primaryNodeId);
        primaryGeoChanged(primaryNode.getOutputPacket(0));
    }

    // cook selected nodes and notify spreadsheet
    for (NodeId selectedId : selectedNodes_)
    {
        cook(selectedId);
        auto& selectedNode = getNode(selectedId);
        selectedGeoChanged(selectedNode.getOutputPacket(0));
    }
}

const std::vector<nt::NodeId>& nt::NetworkManager::getSelectedNodes() { return selectedNodes_; }

void nt::NetworkManager::setSelectedNodes(std::vector<nt::NodeId> nodeIds)
{
    selectedNodes_.clear();
    for (NodeId nodeId : nodeIds)
    {
        if (isValidNode(nodeId))
        {
            selectedNodes_.push_back(nodeId);
            cook(nodeId);
        }
    }
    selectedNodesChanged(selectedNodes_);
}

void nt::NetworkManager::clear()
{
    network_.clear();
    selectedNodes_.clear();
    undoStack_.clear();
    clearDisplayFlag();
    clearPrimaryNode();
    selectedNodesChanged(selectedNodes_);
    networkCleared();
}

void nt::NetworkManager::cook(nt::NodeId nodeId)
{
    std::vector<nt::NodeId> cookOrder = graph().getCookOrder(nodeId);

    for (nt::NodeId cookNodeId : cookOrder)
    {
        nt::Node& node = getNode(cookNodeId);
        if (node.isDirty())
        {
            nt::CookContext context(cookNodeId, nt::nm());
            node.cook(context);
        }
    }
}

nt::Connection nt::NetworkManager::connectNodes(
    NodeId inputNodeId,
    unsigned int inputIndex,
    NodeId outputNodeId,
    unsigned int outputIndex
)
{
    auto updateLock = lockUpdates();

    nt::Connection connection{inputNodeId, inputIndex, outputNodeId, outputIndex};

    // An input slot holds one connection, so replace whatever was there
    if (auto existing = graph().getInputConnection(outputNodeId, outputIndex))
    {
        disconnectNodes(*existing);
    }

    graph().connect(connection);
    getNode(outputNodeId).dirtyNode();
    connectionCreated(connection);

    auto cmd = std::make_unique<ChangeConnectionCommand>(
        inputNodeId,
        inputIndex,
        outputNodeId,
        outputIndex,
        ChangeConnectionCommand::Action::Connect
    );
    undoStack_.push(std::move(cmd));

    return connection;
}

void nt::NetworkManager::disconnectNodes(const nt::Connection& connection)
{
    auto cmd = std::make_unique<ChangeConnectionCommand>(
        connection.sourceNode,
        connection.sourceOutput,
        connection.targetNode,
        connection.targetInput,
        ChangeConnectionCommand::Action::Disconnect
    );
    undoStack_.push(std::move(cmd));

    graph().disconnect(connection);

    // Only the downstream node goes stale, its input changed
    if (isValidNode(connection.targetNode))
    {
        getNode(connection.targetNode).dirtyNode();
    }

    connectionRemoved(connection);
}

std::optional<nt::NodeId> nt::NetworkManager::getDisplayNode() { return displayNode_; }

void nt::NetworkManager::onNodeDirtied(nt::NodeId nodeId, bool dirtyDependents)
{
    if (dirtyDependents)
    {
        std::vector<nt::Unit> dependents = graph().getDependents(nt::Unit{nodeId});
        for (const nt::Unit& dependent : dependents)
        {
            // Dirty dependent node
            nt::Node& dependentNode = getNode(dependent.nodeId);
            dependentNode.dirtyNode(false);

            // Dirty dependent parameter
            if (dependent.isParameter()) dependentNode.parameterChanged(dependent.parm);
        }

        if (nt::UpdateLock::isUnlocked())
        {
            update();
        }
    }
}

void nt::NetworkManager::_reset()
{
    std::cout << "resetting network manager\n";

    network_.clear();
    displayNode_.reset();
}

} // namespace enzo
