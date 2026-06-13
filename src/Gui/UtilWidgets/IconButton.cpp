#include "Gui/UtilWidgets/IconButton.h"
#include "Gui/IconRegistry.h"

#include <QPainter>

namespace {

constexpr int buttonSize = 23;
constexpr int iconSizePx = 15;
constexpr qreal restOpacity = 0.5;
constexpr qreal hoverGrow = 1.1;

} // namespace

enzo::ui::IconButton::IconButton(const std::string& iconName, QWidget* parent) : QPushButton(parent)
{
    setFixedSize(buttonSize, buttonSize);
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    setIcon(IconRegistry::instance().lookup(iconName));
    setIconSize(QSize(iconSizePx, iconSizePx));
}

void enzo::ui::IconButton::enterEvent(QEnterEvent* event)
{
    hovered_ = true;
    update();
    QPushButton::enterEvent(event);
}

void enzo::ui::IconButton::leaveEvent(QEvent* event)
{
    hovered_ = false;
    update();
    QPushButton::leaveEvent(event);
}

void enzo::ui::IconButton::paintEvent(QPaintEvent*)
{
    const QIcon buttonIcon = icon();
    if (buttonIcon.isNull()) return;

    // A disabled button shows only its rest icon, never the hover highlight.
    const bool active = hovered_ && isEnabled();

    QPainter painter(this);
    painter.setOpacity(active ? 1.0 : restOpacity);

    QSize size = iconSize();
    if (active) size *= hoverGrow;

    const QPixmap pixmap = buttonIcon.pixmap(size);
    const QPoint topLeft = rect().center() - QPoint(size.width() / 2, size.height() / 2);
    painter.drawPixmap(topLeft, pixmap);
}
