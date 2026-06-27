#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"

#include <QVariantMap>
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
    nt::nm().createOperator(
        opInfo.value(), "", {static_cast<float>(x), static_cast<float>(y)}
    );
}

} // namespace enzo::ui
