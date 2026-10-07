#pragma once
#include "Graphics/DisplayGeometry.h"
#include <QObject>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui {

/// @brief The view model that turns the display node's output into viewport geometry.
class ViewportViewModel : public QObject
{
    Q_OBJECT
  public:
    explicit ViewportViewModel(QObject* parent = nullptr);

    /// @brief Returns the geometry of the display node's last output, or null when there is none.
    std::shared_ptr<const gfx::DisplayGeometry> getGeometry() const;

  Q_SIGNALS:
    void geometryChanged();

  private:
    std::shared_ptr<const gfx::DisplayGeometry> geometry_;
    boost::signals2::scoped_connection displayGeoSubscription_;
    boost::signals2::scoped_connection networkClearedSubscription_;
};

} // namespace enzo::ui
