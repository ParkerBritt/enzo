#include "Engine/Network/NetworkManager.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/OpInfo.h"
#include "Engine/Network/UpdateLock.h"
#include "Engine/Primitives/Primitive.h"
#include "Engine/UndoRedo/ChangeConnectionCommand.h"
#include "Engine/UndoRedo/CreateNodeCommand.h"
#include "Engine/UndoRedo/DeleteNodeCommand.h"
#include "Engine/UndoRedo/MoveNodeCommand.h"
#include "icecream.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace enzo {

nt::OpId nt::NetworkManager::createOperator(op::OpInfo opInfo, Vector2 position)
{

    OpId opId = ++maxOpId_;
    std::string typeName = opInfo.internalName;

    std::unique_ptr<GeometryOperator> newOp = std::make_unique<GeometryOperator>(maxOpId_, opInfo);
    newOp->setPosition(position);
    newOp->nodeDirtied.connect([this](nt::OpId opId, bool dirtyDependents) {
        onNodeDirtied(opId, dirtyDependents);
    });
    gopStore_.emplace(opId, std::move(newOp));

    operatorCreated(opId);

    auto cmd = std::make_unique<CreateNodeCommand>(opId);
    undoStack_.push(std::move(cmd));

    return opId;
}

void nt::NetworkManager::moveNode(OpId opId, Vector2 newPos, bool skipUndo)
{
    Vector2 oldPos = getGeoOperator(opId).getPosition();
    getGeoOperator(opId).setPosition(newPos);

    if (!skipUndo)
    {
        auto cmd = std::make_unique<MoveNodeCommand>(opId, oldPos, newPos);
        undoStack_.push(std::move(cmd));
    }

    nodePositionChanged(opId, newPos);
}

void nt::NetworkManager::deleteNode(OpId opId)
{
    if (!isValidOp(opId)) return;

    auto updateLock = lockUpdates();

    // Group the disconnects and the node removal into one atomic undo unit
    UndoTransaction transaction(undoStack_);

    // Disconnect first so the reconnects replay after the node is restored on undo
    disconnectOperator(opId);

    // Record and remove the bare node last
    auto cmd = std::make_unique<DeleteNodeCommand>(opId);
    undoStack_.push(std::move(cmd));
    removeOperator(opId, false);
}

void nt::NetworkManager::restoreOperator(OpId opId, op::OpInfo opInfo)
{
    std::unique_ptr<GeometryOperator> newOp = std::make_unique<GeometryOperator>(opId, opInfo);
    newOp->nodeDirtied.connect([this](nt::OpId opId, bool dirtyDependents) {
        onNodeDirtied(opId, dirtyDependents);
    });
    gopStore_.emplace(opId, std::move(newOp));

    if (opId > maxOpId_) maxOpId_ = opId;

    operatorCreated(opId);
}

void nt::NetworkManager::removeOperator(OpId opId, bool removeConnections)
{
    if (!isValidOp(opId)) return;

    auto updateLock = lockUpdates();

    if (removeConnections)
    {
        disconnectOperator(opId);
    }

    // Clear display if this was the display node
    if (displayOp_.has_value() && displayOp_.value() == opId)
    {
        displayOp_.reset();
    }

    // Remove from selection
    auto selIt = std::find(selectedNodes_.begin(), selectedNodes_.end(), opId);
    if (selIt != selectedNodes_.end())
    {
        selectedNodes_.erase(selIt);
        selectedNodesChanged(selectedNodes_);
    }

    // Signal before erasing so listeners can still query the operator
    operatorRemoved(opId);

    graph_.removeNode(opId);
    gopStore_.erase(opId);
}

void nt::NetworkManager::disconnectOperator(OpId opId)
{
    if (!isValidOp(opId)) return;

    // getInputs and getOutputs return copies, so disconnecting while iterating is safe
    for (const nt::Connection& connection : graph_.getInputs(opId))
    {
        disconnectNodes(connection);
    }

    for (const nt::Connection& connection : graph_.getOutputs(opId))
    {
        disconnectNodes(connection);
    }
}

nt::NetworkManager& nt::NetworkManager::getInstance()
{
    static nt::NetworkManager instance;
    return instance;
}

nt::GeometryOperator& nt::NetworkManager::getGeoOperator(nt::OpId opId)
{
    auto it = gopStore_.find(opId);
    if (it == gopStore_.end())
    {
        throw std::out_of_range(
            "OpId: " + std::to_string(opId) + " > max opId: " + std::to_string(maxOpId_) + "\n"
        );
    }
    return *it->second;
}

bool nt::NetworkManager::isValidOp(nt::OpId opId)
{
    auto it = gopStore_.find(opId);
    if (it == gopStore_.end() || it->second == nullptr)
    {
        return false;
    }
    return true;
}

void nt::NetworkManager::setDisplayOp(OpId opId)
{
    displayOp_ = opId;

    cookOp(opId);

    nt::GeometryOperator& displayOp = getGeoOperator(opId);
    displayGeoChanged(displayOp.getOutputPacket(0));
    displayNodeChanged(opId);
}

void nt::NetworkManager::clearDisplayFlag()
{
    displayOp_.reset();
    displayGeoChanged(std::make_shared<const NodePacket>());
    displayNodeChanged(std::nullopt);
}

void nt::NetworkManager::setSelectedNode(OpId opId, bool selected, bool add)
{
    if (add)
    {
        auto idIter = std::find(selectedNodes_.begin(), selectedNodes_.end(), opId);
        if (selected)
        {
            // skip if value is already in selected nodes
            if (idIter != selectedNodes_.end()) return;
            selectedNodes_.push_back(opId);
            cookOp(opId);
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
            selectedNodes_.push_back(opId);
            cookOp(opId);
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
    // cook display op
    if (getDisplayOp().has_value())
    {

        const OpId displayOpId = getDisplayOp().value();
        cookOp(displayOpId);

        auto& displayOp = getGeoOperator(displayOpId);
        displayGeoChanged(displayOp.getOutputPacket(0));
    }

    // cook selected nodes and notify spreadsheet
    for (OpId selectedId : selectedNodes_)
    {
        cookOp(selectedId);
        auto& selectedOp = getGeoOperator(selectedId);
        selectedGeoChanged(selectedOp.getOutputPacket(0));
    }
}

const std::vector<nt::OpId>& nt::NetworkManager::getSelectedNodes() { return selectedNodes_; }

void nt::NetworkManager::setSelectedNodes(std::vector<nt::OpId> opIds)
{
    selectedNodes_.clear();
    for (OpId opId : opIds)
    {
        if (isValidOp(opId))
        {
            selectedNodes_.push_back(opId);
            cookOp(opId);
        }
    }
    selectedNodesChanged(selectedNodes_);
}

void nt::NetworkManager::clear()
{
    gopStore_.clear();
    graph_.clear();
    selectedNodes_.clear();
    maxOpId_ = 0;
    displayOp_.reset();
    undoStack_.clear();
    selectedNodesChanged(selectedNodes_);
    networkCleared();
}

void nt::NetworkManager::cookOp(nt::OpId opId)
{
    std::vector<nt::OpId> cookOrder = graph_.getCookOrder(opId);

    for (nt::OpId cookOpId : cookOrder)
    {
        nt::GeometryOperator& op = getGeoOperator(cookOpId);
        if (op.isDirty())
        {
            op::CookContext context(cookOpId, nt::nm());
            op.cookOp(context);
        }
    }
}

nt::Connection nt::NetworkManager::connectNodes(
    OpId inputOpId,
    unsigned int inputIndex,
    OpId outputOpId,
    unsigned int outputIndex
)
{
    auto updateLock = lockUpdates();

    nt::Connection connection{inputOpId, inputIndex, outputOpId, outputIndex};

    // An input slot holds one connection, so replace whatever was there
    if (auto existing = graph_.getInputConnection(outputOpId, outputIndex))
    {
        disconnectNodes(*existing);
    }

    graph_.connect(connection);
    getGeoOperator(outputOpId).dirtyNode();
    connectionCreated(connection);

    auto cmd = std::make_unique<ChangeConnectionCommand>(
        inputOpId,
        inputIndex,
        outputOpId,
        outputIndex,
        ChangeConnectionCommand::Action::Connect
    );
    undoStack_.push(std::move(cmd));

    return connection;
}

void nt::NetworkManager::disconnectNodes(const nt::Connection& connection)
{
    auto cmd = std::make_unique<ChangeConnectionCommand>(
        connection.sourceOp,
        connection.sourceOutput,
        connection.targetOp,
        connection.targetInput,
        ChangeConnectionCommand::Action::Disconnect
    );
    undoStack_.push(std::move(cmd));

    graph_.disconnect(connection);

    // Only the downstream node goes stale, its input changed
    if (isValidOp(connection.targetOp))
    {
        getGeoOperator(connection.targetOp).dirtyNode();
    }

    connectionRemoved(connection);
}

std::optional<nt::OpId> nt::NetworkManager::getDisplayOp() { return displayOp_; }

void nt::NetworkManager::onNodeDirtied(nt::OpId opId, bool dirtyDependents)
{
    if (dirtyDependents)
    {
        std::vector<nt::Unit> dependents = graph_.getDependents(nt::Unit{opId});
        for (const nt::Unit& dependent : dependents)
        {
            getGeoOperator(dependent.opId).dirtyNode(false);
        }

        if (nt::UpdateLock::isUnlocked())
        {
            update();
        }
    }
}

#ifdef UNIT_TEST
void nt::NetworkManager::_reset()
{
    std::cout << "resetting network manager\n";

    gopStore_.clear();
    graph_.clear();
    maxOpId_ = 0;
    displayOp_.reset();
}
#endif

// std::unordered_map<nt::OpId, std::unique_ptr<nt::GeometryOperator>>
// nt::NetworkManager::gopStore_;

} // namespace enzo
