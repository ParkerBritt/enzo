#pragma once
#include <QGraphicsItem>
#include "Gui/Network/SocketGraphic.h"

#include <QPainter>

class NodeEdgeGraphic
: public QGraphicsItem
{
public:
    NodeEdgeGraphic(SocketGraphic* socket1, SocketGraphic* socket2, QGraphicsItem *parent = nullptr);
    ~NodeEdgeGraphic();
    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    // void setColor(QColor color);
    // void useDefaultColor();
    void updateDeleteHighlightPen();
    void setPos(QPointF pos1, QPointF pos2);
    void setStartPos(QPointF pos);
    void setEndPos(QPointF pos);
    void cleanUp();
    void setDeleteHighlight(bool state);
    QPen deleteHighlightPen_;
    QPen defaultPen_;

    bool deleteHighlight_=false;

    QPointF closestPoint(QPointF startPos);

private:
    SocketGraphic* socket1_;
    SocketGraphic* socket2_;
    QColor color_;
    QColor defaultColor_;
    QPointF pos1_;
    QPointF pos2_;
    QPainterPath path_;
    QRectF boundRect_;
    qreal padding_=40;
    QPointF hoverPos_;

    void updatePath();
protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
};

