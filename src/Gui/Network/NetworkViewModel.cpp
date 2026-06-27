#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/UndoRedo/ChangePrimaryNodeCommand.h"
#include "Engine/UndoRedo/ChangeSelectionCommand.h"
#include "Engine/UndoRedo/UndoStack.h"

#include <QVariantMap>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>

namespace enzo::ui {

NetworkViewModel::NetworkViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    operatorCreatedConnection_ =
        network.operatorCreated.connect([this](nt::OpId opId) { nodes_.addNode(opId); });

    operatorRemovedConnection_ =
        network.operatorRemoved.connect([this](nt::OpId opId) { nodes_.removeNode(opId); });

    networkClearedConnection_ = network.networkCleared.connect([this]() { nodes_.clear(); });

    selectedNodesConnection_ =
        network.selectedNodesChanged.connect([this](std::vector<nt::OpId> selectedNodeIds) {
            nodes_.setSelection(selectedNodeIds);
        });

    primaryNodeConnection_ =
        network.primaryNodeChanged.connect([this](std::optional<nt::OpId> primaryId) {
            nodes_.setPrimary(primaryId);
        });

    // Catches any operators that already exist before the subscriptions are live.
    nodes_.resetFromNetwork();
}

QAbstractListModel* NetworkViewModel::nodes() { return &nodes_; }

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
