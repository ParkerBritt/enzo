#include "Gui/Timeline/TimelineViewModel.h"
#include "Engine/Network/NetworkManager.h"

namespace enzo::ui {

TimelineViewModel::TimelineViewModel(QObject* parent) : QObject(parent)
{
    frameSubscription_ = nt::nm().frameChanged.connect([this](floatT) { Q_EMIT frameChanged(); });
    frameRangeSubscription_ =
        nt::nm().frameRangeChanged.connect([this](intT, intT) { Q_EMIT frameRangeChanged(); });
    fpsSubscription_ = nt::nm().fpsChanged.connect([this](floatT) { Q_EMIT fpsChanged(); });
}

qreal TimelineViewModel::frame() const { return nt::nm().getFrame(); }

int TimelineViewModel::startFrame() const { return nt::nm().getStartFrame(); }

int TimelineViewModel::endFrame() const { return nt::nm().getEndFrame(); }

qreal TimelineViewModel::fps() const { return nt::nm().getFps(); }

void TimelineViewModel::setFrame(qreal frame) { nt::nm().setFrame(static_cast<floatT>(frame)); }

void TimelineViewModel::setStartFrame(int frame) { nt::nm().setStartFrame(frame); }

void TimelineViewModel::setEndFrame(int frame) { nt::nm().setEndFrame(frame); }

void TimelineViewModel::setFps(qreal fps) { nt::nm().setFps(static_cast<floatT>(fps)); }

} // namespace enzo::ui
