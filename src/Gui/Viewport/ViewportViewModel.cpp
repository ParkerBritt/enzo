#include "Gui/Viewport/ViewportViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodePacket.h"

namespace enzo::ui {

ViewportViewModel::ViewportViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    displayGeoSubscription_ =
        network.displayGeoChanged.connect([this](std::shared_ptr<const NodePacket> packet) {
            geometry_ = packet ? gfx::buildDisplayGeometry(*packet) : nullptr;
            Q_EMIT geometryChanged();
        });

    networkClearedSubscription_ = network.networkCleared.connect([this]() {
        geometry_ = nullptr;
        Q_EMIT geometryChanged();
    });
}

std::shared_ptr<const gfx::DisplayGeometry> ViewportViewModel::getGeometry() const
{
    return geometry_;
}

} // namespace enzo::ui
