#include "Panel.h"
#include <QPainter>
#include <QPainterPath>
#include <QLayout>

Panel::Panel(QWidget *parent)
: QWidget(parent)
{
    cornerOverlay_ = new CornerOverlay(this);
    cornerOverlay_->raise();
}

void Panel::enterEvent(QEnterEvent *event)
{
    // Steal focus on hover
    setFocus(Qt::MouseFocusReason);
    QWidget::enterEvent(event);
}

void Panel::setBorderColor(QColor color)
{
    cornerOverlay_->setBorderColor(color);

}

void Panel::resizeEvent(QResizeEvent *event)
{
    cornerOverlay_->setGeometry(this->layout()->contentsRect());
    cornerOverlay_->raise();
}
