#include "Gui/Viewport/ViewportViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodePacket.h"
#include "Gui/PathNames.h"

namespace enzo::ui {

ViewportViewModel::ViewportViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    displayNodeSubscription_ =
        network.displayNodeChanged.connect([this](std::optional<nt::NodeId> displayId) {
            nodePath_ = displayId.has_value() ? getPathNames(nt::nm().getNode(*displayId).getPath())
                                              : QStringList();
            Q_EMIT nodePathChanged();
        });

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

QStringList ViewportViewModel::cameraPaths() const
{
    QStringList paths;
    if (!geometry_) return paths;

    for (const std::string& path : geometry_->cameraPaths)
        paths.append(QString::fromStdString(path));
    return paths;
}

} // namespace enzo::ui
