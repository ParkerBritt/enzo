#pragma once
#include <QObject>
#include <boost/signals2/connection.hpp>

namespace enzo::ui {

/// @brief View-model backing the timeline bar.
///
/// Exposes the scene frame, the playback range and the frame rate as Qt
/// properties.
class TimelineViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal frame READ frame WRITE setFrame NOTIFY frameChanged)
    Q_PROPERTY(int startFrame READ startFrame WRITE setStartFrame NOTIFY frameRangeChanged)
    Q_PROPERTY(int endFrame READ endFrame WRITE setEndFrame NOTIFY frameRangeChanged)
    Q_PROPERTY(qreal fps READ fps WRITE setFps NOTIFY fpsChanged)

  public:
    explicit TimelineViewModel(QObject* parent = nullptr);

    /// @brief Returns the frame the scene sits on.
    qreal frame() const;

    /// @brief Returns the first frame of the playback range.
    int startFrame() const;

    /// @brief Returns the last frame of the playback range.
    int endFrame() const;

    /// @brief Returns the playback frame rate.
    qreal fps() const;

    void setFrame(qreal frame);
    void setStartFrame(int frame);
    void setEndFrame(int frame);
    void setFps(qreal fps);

  Q_SIGNALS:
    void frameChanged();
    void frameRangeChanged();
    void fpsChanged();

  private:
    boost::signals2::scoped_connection frameSubscription_;
    boost::signals2::scoped_connection frameRangeSubscription_;
    boost::signals2::scoped_connection fpsSubscription_;
};

} // namespace enzo::ui
