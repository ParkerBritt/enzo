#pragma once

#include <QList>
#include <QObject>

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

    /// @brief Returns whether the scale differs from the saved one.
    bool hasPendingChanges() const;

    /// @brief Saves the previewed scale.
    Q_INVOKABLE void apply();

    /// @brief Restores the saved scale.
    Q_INVOKABLE void revert();

  Q_SIGNALS:
    void scaleChanged();
    void hasPendingChangesChanged();

  private:
    int scaleIndex_ = 0;
    int savedScaleIndex_ = 0;
};

} // namespace enzo::ui
