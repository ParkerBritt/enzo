#include "Gui/Components/IconCursor.h"
#include "Gui/Components/IconImageProvider.h"

#include <QCursor>
#include <QImage>
#include <QPixmap>
#include <QQuickWindow>

namespace enzo::ui {

namespace {

// The side length of a Lucide icon's view box.
constexpr qreal lucideViewSize = 24;

} // namespace

IconCursor::IconCursor(QQuickItem* parent) : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::NoButton);
    connect(this, &IconCursor::cursorChanged, this, &IconCursor::applyCursor);
    connect(this, &QQuickItem::windowChanged, this, &IconCursor::applyCursor);
}

void IconCursor::applyCursor()
{
    if (!active_ || name_.isEmpty())
    {
        unsetCursor();
        return;
    }

    const qreal pixelRatio = window() ? window()->devicePixelRatio() : 1.0;
    const int pixelSize = qRound(size_ * pixelRatio);
    const QImage image = renderIcon(name_, color_, QSize(pixelSize, pixelSize));
    if (image.isNull())
    {
        unsetCursor();
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(pixelRatio);

    const qreal unitScale = size_ / lucideViewSize;
    setCursor(QCursor(pixmap, qRound(hotSpot_.x() * unitScale), qRound(hotSpot_.y() * unitScale)));
}

} // namespace enzo::ui
