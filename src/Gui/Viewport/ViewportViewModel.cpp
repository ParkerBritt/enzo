#include "Gui/Viewport/ViewportViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodePacket.h"

namespace enzo::ui
{

ViewportViewModel::ViewportViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    // The display node delivers its cooked geometry directly.
    displayGeoSubscription_ =
        network.displayGeoChanged.connect([this](std::shared_ptr<const NodePacket> packet) {
            packet_ = std::move(packet);
            Q_EMIT geometryChanged();
        });

    networkClearedSubscription_ = network.networkCleared.connect([this]() {
        packet_ = nullptr;
        Q_EMIT geometryChanged();
    });
}

std::shared_ptr<const NodePacket> ViewportViewModel::currentGeometry() const { return packet_; }

} // namespace enzo::ui
