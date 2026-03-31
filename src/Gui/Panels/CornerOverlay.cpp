#include "CornerOverlay.h"
#include <QPainterPath>
#include <QPainter>

CornerOverlay::CornerOverlay(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
}
void CornerOverlay::setBorderColor(QColor color)
{
    borderPen_ = QPen(color);
}

void CornerOverlay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    constexpr float radius = 10;
    QPainterPath outer;
    outer.addRect(rect());
    QPainterPath inner;
    inner.addRoundedRect(rect(), radius, radius);

    painter.fillPath(outer - inner,
        palette().color(QPalette::Window));

    painter.setPen(borderPen_);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
}

