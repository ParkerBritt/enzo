#pragma once
#include "Engine/Network/UpdateLock.h"

#include <QTimer>
#include <boost/signals2/connection.hpp>

#include <optional>

namespace enzo::ui {

class TimelineViewModel;

/// @brief The throttle that holds the network's updates and cooks them at most 75 times a second.
///
/// @note During timeline playback held updates cook on the next event loop pass.
class CookThrottle
{
  public:
    explicit CookThrottle(const TimelineViewModel& timeline);

  private:
    /// @brief Cooks the held updates and starts the wait before the next cook.
    void cookHeldUpdates();

    const TimelineViewModel& timeline_;
    // The lock that holds every network update, so dragging a slider parameter does not cook
    // hundreds of times a second.
    std::optional<nt::UpdateLock> cookLock_;
    QTimer cookWaitTimer_;
    boost::signals2::scoped_connection updateHeldSubscription_;
};

} // namespace enzo::ui
