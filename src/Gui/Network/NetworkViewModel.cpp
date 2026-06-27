#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/UndoRedo/ChangeSelectionCommand.h"

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
    std::vector<nt::OpId> prev = network.getSelectedNodes();

    std::vector<nt::OpId> next;
    if (additive)
    {
        next = prev;
        const auto found = std::find(next.begin(), next.end(), opId);
        if (found != next.end())
            next.erase(found);
        else
            next.push_back(opId);
    }
    else
    {
        next = {opId};
    }

    if (next == prev) return;

    network.undoStack().push(std::make_unique<nt::ChangeSelectionCommand>(prev, next));
    network.setSelectedNodes(next);
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
