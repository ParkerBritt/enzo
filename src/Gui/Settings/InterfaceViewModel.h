#pragma once

#include <QList>
#include <QObject>
#include <QStringList>

namespace enzo::ui {

/// @brief View-model backing the interface page of the settings window.
///
/// @note An edit previews straight away and is saved only on apply.
class InterfaceViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<qreal> scales READ scales CONSTANT)
    Q_PROPERTY(int scaleIndex READ scaleIndex WRITE setScaleIndex NOTIFY scaleChanged)
    Q_PROPERTY(qreal scale READ scale NOTIFY scaleChanged)
    Q_PROPERTY(QStringList animationSpeedLabels READ animationSpeedLabels CONSTANT)
    Q_PROPERTY(int animationSpeedIndex READ animationSpeedIndex WRITE setAnimationSpeedIndex NOTIFY animationSpeedChanged)
    Q_PROPERTY(qreal durationScale READ durationScale NOTIFY animationSpeedChanged)
    Q_PROPERTY(bool hasPendingChanges READ hasPendingChanges NOTIFY hasPendingChangesChanged)

  public:
    explicit InterfaceViewModel(QObject* parent = nullptr);

    /// @brief Returns the scales the user can pick from, smallest first.
    QList<qreal> scales() const;

    /// @brief Returns the index of the user's scale in scales().
    int scaleIndex() const;

    /// @brief Sets the user's scale by its index in scales() and previews it.
    void setScaleIndex(int index);

    /// @brief Returns the user's scale.
    qreal scale() const;

    /// @brief Returns the names of the animation speeds the user can pick from, from off to slowest.
    QStringList animationSpeedLabels() const;

    /// @brief Returns the index of the user's animation speed in animationSpeedLabels().
    int animationSpeedIndex() const;

    /// @brief Sets the user's animation speed by its index in animationSpeedLabels() and previews it.
    void setAnimationSpeedIndex(int index);

    /// @brief Returns the factor every animation duration is multiplied by, 0 when animations are off.
    qreal durationScale() const;

    /// @brief Returns whether any setting differs from the saved one.
    bool hasPendingChanges() const;

    /// @brief Saves the previewed settings.
    Q_INVOKABLE void apply();

    /// @brief Restores the saved settings.
    Q_INVOKABLE void revert();

  Q_SIGNALS:
    void scaleChanged();
    void animationSpeedChanged();
    void hasPendingChangesChanged();

  private:
    int scaleIndex_ = 0;
    int savedScaleIndex_ = 0;
    int animationSpeedIndex_ = 0;
    int savedAnimationSpeedIndex_ = 0;
};

} // namespace enzo::ui
