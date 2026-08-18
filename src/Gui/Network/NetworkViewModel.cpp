#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/UndoRedo/ChangeDisplayFlagCommand.h"
#include "Engine/UndoRedo/ChangePrimaryNodeCommand.h"
#include "Engine/UndoRedo/ChangeSelectionCommand.h"
#include "Engine/UndoRedo/UndoStack.h"

#include <QPointF>
#include <QVariantMap>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>

namespace enzo::ui {

NetworkViewModel::NetworkViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    nodeCreatedSubscription_ =
        network.nodeCreated.connect([this](nt::NodeId nodeId) { nodes_.addNode(nodeId); });

    nodeRemovedSubscription_ =
        network.nodeRemoved.connect([this](nt::NodeId nodeId) { nodes_.removeNode(nodeId); });

    networkClearedSubscription_ = network.networkCleared.connect([this]() {
        nodes_.clear();
        edges_.clear();
    });

    selectedNodesSubscription_ =
        network.selectedNodesChanged.connect([this](std::vector<nt::NodeId> selectedNodeIds) {
            nodes_.setSelection(selectedNodeIds);
        });

    primaryNodeSubscription_ =
        network.primaryNodeChanged.connect([this](std::optional<nt::NodeId> primaryId) {
            nodes_.setPrimary(primaryId);
        });

    displayNodeSubscription_ =
        network.displayNodeChanged.connect([this](std::optional<nt::NodeId> displayId) {
            nodes_.setDisplay(displayId);
        });

    nodePositionSubscription_ =
        network.nodePositionChanged.connect([this](nt::NodeId nodeId, Vector2 pos) {
            nodes_.setPosition(nodeId, pos.x(), pos.y());
        });

    connectionCreatedSubscription_ =
        network.connectionCreated.connect([this](nt::Connection connection) {
            edges_.addEdge(connection);
        });

    connectionRemovedSubscription_ =
        network.connectionRemoved.connect([this](nt::Connection connection) {
            edges_.removeEdge(connection);
        });

    // Catches any graph state that already exists before the subscriptions are live.
    nodes_.resetFromNetwork();
    edges_.resetFromNetwork();
}

QAbstractListModel* NetworkViewModel::nodes() { return &nodes_; }

QAbstractListModel* NetworkViewModel::edges() { return &edges_; }

qreal NetworkViewModel::getNodeWidth() const { return NodeListModel::nodeWidth; }

qreal NetworkViewModel::getNodeHeight() const { return NodeListModel::nodeHeight; }

QVariantList NetworkViewModel::getNodeTypes() const
{
    QVariantList list;
    for (const nt::NodeType& info : nt::NodeTypeTable::getData())
    {
        QVariantMap entry;
        entry["label"] = QString::fromStdString(info.displayName);
        entry["name"] = QString::fromStdString(info.getFullName());
        list.append(entry);
    }
    return list;
}

void NetworkViewModel::createNode(const QString& fullName, qreal x, qreal y)
{
    const nt::NodeType& nodeType = nt::NodeTypeTable::requireNodeType(fullName.toStdString());
    nt::nm().createNode(nodeType, Path("/"), "", {static_cast<float>(x), static_cast<float>(y)});
}

void NetworkViewModel::selectNode(qulonglong nodeId, bool additive)
{
    auto& network = nt::nm();

    std::vector<nt::NodeId> prevSelection = network.getSelectedNodes();
    std::vector<nt::NodeId> nextSelection;
    if (additive)
    {
        nextSelection = prevSelection;
        const auto found = std::find(nextSelection.begin(), nextSelection.end(), nodeId);
        if (found != nextSelection.end())
            nextSelection.erase(found);
        else
            nextSelection.push_back(nodeId);
    }
    else
    {
        nextSelection = {nodeId};
    }

    std::optional<nt::NodeId> prevPrimary = network.getPrimaryNode();

    // A click changes selection and primary together, so they undo as one unit.
    nt::UndoTransaction transaction(network.undoStack());

    if (nextSelection != prevSelection)
    {
        network.undoStack().push(
            std::make_unique<nt::ChangeSelectionCommand>(prevSelection, nextSelection)
        );
        network.setSelectedNodes(nextSelection);
    }

    if (prevPrimary != nodeId)
    {
        network.undoStack().push(
            std::make_unique<nt::ChangePrimaryNodeCommand>(prevPrimary, nodeId)
        );
        network.setPrimaryNode(nodeId);
    }
}

void NetworkViewModel::stageSelectionMove(qreal dx, qreal dy)
{
    nodes_.moveSelectedBy(static_cast<float>(dx), static_cast<float>(dy));
}

void NetworkViewModel::commitSelectionMove()
{
    auto& network = nt::nm();

    std::vector<nt::NodeId> selected = network.getSelectedNodes();
    if (selected.empty()) return;

    nt::UndoTransaction transaction(network.undoStack());
    for (nt::NodeId nodeId : selected)
    {
        const QPointF position = nodes_.getPosition(nodeId);
        network.moveNode(
            nodeId,
            {static_cast<float>(position.x()), static_cast<float>(position.y())}
        );
    }
}

void NetworkViewModel::undo() { nt::nm().undoStack().undo(); }

void NetworkViewModel::redo() { nt::nm().undoStack().redo(); }

void NetworkViewModel::deleteSelected()
{
    auto& network = nt::nm();

    // A copy is taken because deleting a node mutates the live selection.
    std::vector<nt::NodeId> selected = network.getSelectedNodes();
    if (selected.empty()) return;

    nt::UndoTransaction transaction(network.undoStack());
    for (nt::NodeId nodeId : selected)
        network.deleteNode(nodeId);
}

void NetworkViewModel::connectNodes(
    qulonglong sourceNode,
    int sourceOutput,
    qulonglong targetNode,
    int targetInput
)
{
    // A node cannot feed itself.
    if (sourceNode == targetNode) return;

    // The engine pushes its own undo command and emits connectionCreated, which
    // the edge model already listens for, so the link appears through that path.
    nt::nm().connectNodes(sourceNode, sourceOutput, targetNode, targetInput);
}

void NetworkViewModel::removeLink(int linkIndex)
{
    // The engine emits connectionRemoved, which the edge model already listens for.
    if (auto connection = edges_.connectionAt(linkIndex)) nt::nm().disconnectNodes(*connection);
}

QVariantMap NetworkViewModel::getLinkEndpoints(int linkIndex) const
{
    const std::optional<nt::Connection> connection = edges_.connectionAt(linkIndex);
    if (!connection) return {};
    return {
        {"sourceNode", static_cast<qulonglong>(connection->sourceNode)},
        {"sourceOutput", static_cast<int>(connection->sourceOutput)},
        {"targetNode", static_cast<qulonglong>(connection->targetNode)},
        {"targetInput", static_cast<int>(connection->targetInput)},
    };
}

void NetworkViewModel::setDisplayNode(qulonglong nodeId)
{
    auto& network = nt::nm();

    std::optional<nt::NodeId> prev = network.getDisplayNode();
    if (prev == nodeId) return;

    network.undoStack().push(std::make_unique<nt::ChangeDisplayFlagCommand>(prev, nodeId));
    network.setDisplayNode(nodeId);
}

void NetworkViewModel::clearSelection()
{
    auto& network = nt::nm();
    std::vector<nt::NodeId> prev = network.getSelectedNodes();
    if (prev.empty()) return;

    network.undoStack().push(
        std::make_unique<nt::ChangeSelectionCommand>(prev, std::vector<nt::NodeId>{})
    );
    network.setSelectedNodes({});
}

} // namespace enzo::ui
