#pragma once
#include "Graphics/DisplayGeometry.h"
#include <QObject>
#include <QStringList>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui {

/// @brief The view model that turns the display node's output into viewport geometry.
class ViewportViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList nodePath READ nodePath NOTIFY nodePathChanged)
    Q_PROPERTY(QStringList cameraPaths READ cameraPaths NOTIFY geometryChanged)
  public:
    explicit ViewportViewModel(QObject* parent = nullptr);

    /// @brief Returns the geometry of the display node's last output, or null when there is none.
    std::shared_ptr<const gfx::DisplayGeometry> getGeometry() const;

    /// @brief Returns the names along the display node's path, or an empty list when no node is
    /// displayed.
    QStringList nodePath() const { return nodePath_; }

    /// @brief Returns the path of each camera in the geometry, in the order of its camera transforms.
    QStringList cameraPaths() const;

  Q_SIGNALS:
    void geometryChanged();
    void nodePathChanged();

  private:
    std::shared_ptr<const gfx::DisplayGeometry> geometry_;
    QStringList nodePath_;
    boost::signals2::scoped_connection displayNodeSubscription_;
    boost::signals2::scoped_connection displayGeoSubscription_;
    boost::signals2::scoped_connection networkClearedSubscription_;
};

} // namespace enzo::ui
