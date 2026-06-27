#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"

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

} // namespace enzo::ui
