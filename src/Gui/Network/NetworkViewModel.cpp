#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
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

    operatorCreatedSubscription_ =
        network.operatorCreated.connect([this](nt::OpId opId) { nodes_.addNode(opId); });

    operatorRemovedSubscription_ =
        network.operatorRemoved.connect([this](nt::OpId opId) { nodes_.removeNode(opId); });

    networkClearedSubscription_ = network.networkCleared.connect([this]() { nodes_.clear(); });

    selectedNodesSubscription_ =
        network.selectedNodesChanged.connect([this](std::vector<nt::OpId> selectedNodeIds) {
            nodes_.setSelection(selectedNodeIds);
        });

    primaryNodeSubscription_ =
        network.primaryNodeChanged.connect([this](std::optional<nt::OpId> primaryId) {
            nodes_.setPrimary(primaryId);
        });

    displayNodeSubscription_ =
        network.displayNodeChanged.connect([this](std::optional<nt::OpId> displayId) {
            nodes_.setDisplay(displayId);
        });

    nodePositionSubscription_ =
        network.nodePositionChanged.connect([this](nt::OpId opId, Vector2 pos) {
            nodes_.setPosition(opId, pos.x(), pos.y());
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
    for (const op::OpInfo& info : op::OperatorTable::getData())
    {
        QVariantMap entry;
        entry["label"] = QString::fromStdString(info.displayName);
        entry["name"] = QString::fromStdString(info.internalName);
        list.append(entry);
    }
    return list;
}

void NetworkViewModel::createNode(const QString& internalName, qreal x, qreal y)
{
    std::optional<op::OpInfo> opInfo = op::OperatorTable::getOpInfo(internalName.toStdString());
    if (!opInfo.has_value())
    {
        throw std::runtime_error("Couldn't find op info for: " + internalName.toStdString());
    }
    nt::nm().createOperator(opInfo.value(), "", {static_cast<float>(x), static_cast<float>(y)});
}

void NetworkViewModel::selectNode(qulonglong opId, bool additive)
{
    auto& network = nt::nm();

    std::vector<nt::OpId> prevSelection = network.getSelectedNodes();
    std::vector<nt::OpId> nextSelection;
    if (additive)
    {
        nextSelection = prevSelection;
        const auto found = std::find(nextSelection.begin(), nextSelection.end(), opId);
        if (found != nextSelection.end())
            nextSelection.erase(found);
        else
            nextSelection.push_back(opId);
    }
    else
    {
        nextSelection = {opId};
    }

    std::optional<nt::OpId> prevPrimary = network.getPrimaryNode();

    // A click changes selection and primary together, so they undo as one unit.
    nt::UndoTransaction transaction(network.undoStack());

    if (nextSelection != prevSelection)
    {
        network.undoStack().push(
            std::make_unique<nt::ChangeSelectionCommand>(prevSelection, nextSelection)
        );
        network.setSelectedNodes(nextSelection);
    }

    if (prevPrimary != opId)
    {
        network.undoStack().push(std::make_unique<nt::ChangePrimaryNodeCommand>(prevPrimary, opId));
        network.setPrimaryNode(opId);
    }
}

void NetworkViewModel::stageSelectionMove(qreal dx, qreal dy)
{
    nodes_.moveSelectedBy(static_cast<float>(dx), static_cast<float>(dy));
}

void NetworkViewModel::commitSelectionMove()
{
    auto& network = nt::nm();

    std::vector<nt::OpId> selected = network.getSelectedNodes();
    if (selected.empty()) return;

    nt::UndoTransaction transaction(network.undoStack());
    for (nt::OpId opId : selected)
    {
        const QPointF position = nodes_.getPosition(opId);
        network.moveNode(
            opId,
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
    std::vector<nt::OpId> selected = network.getSelectedNodes();
    if (selected.empty()) return;

    nt::UndoTransaction transaction(network.undoStack());
    for (nt::OpId opId : selected)
        network.deleteNode(opId);
}

void NetworkViewModel::connectNodes(
    qulonglong sourceOp,
    int sourceOutput,
    qulonglong targetOp,
    int targetInput
)
{
    // A node cannot feed itself.
    if (sourceOp == targetOp) return;

    // The engine pushes its own undo command and emits connectionCreated, which
    // the edge model already listens for, so the link appears through that path.
    nt::nm().connectNodes(sourceOp, sourceOutput, targetOp, targetInput);
}

void NetworkViewModel::setDisplayNode(qulonglong opId)
{
    auto& network = nt::nm();

    std::optional<nt::OpId> prev = network.getDisplayOp();
    if (prev == opId) return;

    network.undoStack().push(std::make_unique<nt::ChangeDisplayFlagCommand>(prev, opId));
    network.setDisplayOp(opId);
}

void NetworkViewModel::clearSelection()
{
    auto& network = nt::nm();
    std::vector<nt::OpId> prev = network.getSelectedNodes();
    if (prev.empty()) return;

    network.undoStack().push(
        std::make_unique<nt::ChangeSelectionCommand>(prev, std::vector<nt::OpId>{})
    );
    network.setSelectedNodes({});
}

} // namespace enzo::ui
