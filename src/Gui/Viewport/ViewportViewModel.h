#pragma once
#include <QObject>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo
{
class NodePacket;
}

namespace enzo::ui
{

/// @brief View-model feeding the viewport the display node geometry.
///
/// Subscribes to the engine display signals and holds the packet the viewport
/// item reads on its next sync. This is the boundary that turns the engine
/// boost signals into something the render item observes.
class ViewportViewModel : public QObject
{
    Q_OBJECT
  public:
    explicit ViewportViewModel(QObject* parent = nullptr);

    /// @brief Geometry the display node last produced, or null when none.
    std::shared_ptr<const enzo::NodePacket> currentGeometry() const;

  Q_SIGNALS:
    void geometryChanged();

  private:
    std::shared_ptr<const enzo::NodePacket> packet_;
    boost::signals2::scoped_connection displayGeoSubscription_;
    boost::signals2::scoped_connection networkClearedSubscription_;
};

} // namespace enzo::ui
