#pragma once
#include "LegacyGui/Network/SocketGraphic.h"
#include <QGraphicsItem>

#include <QPainter>

class FloatingEdgeGraphic : public QGraphicsItem
{
  public:
    FloatingEdgeGraphic(SocketGraphic* socket1, QGraphicsItem* parent = nullptr);
    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void setFloatPos(QPointF floatPos);

  private:
    SocketGraphic* socket1_;
    QPointF floatPos_;
};
