#include "Gui/Network/CookThrottle.h"
#include "Engine/Network/NetworkManager.h"
#include "Gui/Timeline/TimelineViewModel.h"

namespace enzo::ui {

namespace {

// The shortest time between the starts of two held cooks.
constexpr int minimumCookIntervalMs = 13;

} // namespace

CookThrottle::CookThrottle(const TimelineViewModel& timeline)
    : timeline_(timeline), cookLock_(nt::nm().lockUpdates())
{
    // Setup the timer that throttles cooks by holding a lock and releasing it every 13ms
    cookWaitTimer_.setSingleShot(true);
    cookWaitTimer_.setTimerType(Qt::PreciseTimer);
    QObject::connect(&cookWaitTimer_, &QTimer::timeout, &cookWaitTimer_, [this]() {
        cookHeldUpdates();
    });

    // Start the timer when an update is held
    updateHeldSubscription_ = nt::nm().updateHeld.connect([this]() {
        if (!cookWaitTimer_.isActive()) cookWaitTimer_.start(0);
    });
}

void CookThrottle::cookHeldUpdates()
{
    if (!nt::UpdateLock::isUpdateHeld()) return;

    // Waits the minimum interval before the next cook, or no time during playback.
    cookWaitTimer_.start(timeline_.isPlaying() ? 0 : minimumCookIntervalMs);

    // Releases the lock so the held updates cook, then takes it again.
    cookLock_.reset();
    cookLock_.emplace(nt::nm().lockUpdates());
}

} // namespace enzo::ui
