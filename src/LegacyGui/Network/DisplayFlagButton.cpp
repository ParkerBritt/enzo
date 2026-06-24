#include "LegacyGui/Network/DisplayFlagButton.h"
#include "LegacyGui/Network/NodeGraphic.h"
#include <QPainter>
#include <qgraphicsitem.h>
#include <qnamespace.h>

DisplayFlagButton::DisplayFlagButton(QGraphicsItem* parent) : QGraphicsItem(parent)
{
    constexpr float width = 8;
    const float height = static_cast<NodeGraphic*>(parent)->getBodyRect().height() * 0.8;
    baseRect_ = QRectF(-width / 2.0f, -height / 2.0f, width, height);
    setAcceptHoverEvents(true);

    disabledBrush_ = QBrush(disabledColor_);
    enabledBrush_ = QBrush(enabledColor_);
    hoveredEnabledBrush_ = QBrush(enzo::style::displayFlag::hoveredEnabledColor);
    hoveredDisabledBrush_ = QBrush(hoveredColor_);
}

void DisplayFlagButton::setEnabled(bool enabled)
{
    enabled_ = enabled;
    update();
}

void DisplayFlagButton::paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget
)
{
    painter->setPen(Qt::NoPen);
    // painter->setBrush(QBrush(disabledColor_));
    QBrush usedBrush;
    if (enabled_)
    {
        if (hovered_)
        {
            usedBrush = hoveredEnabledBrush_;
        }
        else
        {
            usedBrush = enabledBrush_;
        }
    }
    else
    {
        if (hovered_)
        {
            usedBrush = hoveredDisabledBrush_;
        }
        else
        {
            usedBrush = disabledBrush_;
        }
    }
    painter->setBrush(usedBrush);
    constexpr float roundRad = 3;
    painter->drawRoundedRect(baseRect_, roundRad, roundRad);
}

QRectF DisplayFlagButton::boundingRect() const { return baseRect_; }

float DisplayFlagButton::getWidth() { return baseRect_.width(); }

void DisplayFlagButton::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    hovered_ = true;
    update();
}

void DisplayFlagButton::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    hovered_ = false;
    update();
}
