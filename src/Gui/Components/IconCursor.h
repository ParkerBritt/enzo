#pragma once

#include <QColor>
#include <QPointF>
#include <QQuickItem>
#include <QString>

namespace enzo::ui {

/// @brief Shows a tinted icon from the bundled Lucide set as the mouse cursor over the item.
/// @note The item takes no input, so it can sit over a MouseArea and only change the cursor.
class IconCursor : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name MEMBER name_ NOTIFY cursorChanged)
    Q_PROPERTY(QColor color MEMBER color_ NOTIFY cursorChanged)
    Q_PROPERTY(qreal size MEMBER size_ NOTIFY cursorChanged)

    // The point of the icon that lands on the cursor position, in icon units of 0 to 24.
    Q_PROPERTY(QPointF hotSpot MEMBER hotSpot_ NOTIFY cursorChanged)

    // Whether the icon replaces the cursor. The cursor below shows through while false.
    Q_PROPERTY(bool active MEMBER active_ NOTIFY cursorChanged)

  public:
    explicit IconCursor(QQuickItem* parent = nullptr);

  Q_SIGNALS:
    void cursorChanged();

  private:
    /// @brief Sets or clears the item's cursor to match its properties.
    void applyCursor();

    QString name_;
    QColor color_ = Qt::white;
    qreal size_ = 24;
    QPointF hotSpot_;
    bool active_ = false;
};

} // namespace enzo::ui
