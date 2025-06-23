#include "gui/network/FloatingEdgeGraphic.h"
#include <QTextDocument>
#include "gui/network/SocketGraphic.h"
#include <iostream>
#include <qgraphicsitem.h>
#include <algorithm>

FloatingEdgeGraphic::FloatingEdgeGraphic(SocketGraphic* socket1, QGraphicsItem *parent)
: QGraphicsItem(parent), socket1_{socket1}
{
    floatPos_ = socket1_->scenePos();
    setZValue(-1);
}

QRectF FloatingEdgeGraphic::boundingRect() const
{
    auto boundRect = QRectF(socket1_->scenePos(), floatPos_).normalized();
    return boundRect;
}

void FloatingEdgeGraphic::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // auto pen = QPen("white");

    QLinearGradient gradient(socket1_->scenePos(), floatPos_);
    gradient.setColorAt(0.0, QColor(255, 255, 255, 255));
    gradient.setColorAt(1.0, QColor(255, 255, 255, 50));

    QPen pen(QBrush(gradient), 1);

    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    // painter->drawLine(socket1_->scenePos(),floatPos_);

    QPointF pos1 = socket1_->getIO()==SocketGraphic::SocketType::Input ? socket1_->scenePos() : floatPos_;
    QPointF pos2 = socket1_->getIO()==SocketGraphic::SocketType::Input ? floatPos_ : socket1_->scenePos();
    float dist = std::sqrt(std::pow(pos1.x()-pos2.x(),2)+std::pow(pos1.y()-pos2.y(),2));
    std::cout << "dist: " << dist << "\n";
    float cubicStrength = dist*0.5;
    cubicStrength = std::clamp(cubicStrength, 0.0f, 40.0f);
    QPainterPath path;
    path.moveTo(pos1);
    path.cubicTo(pos1-QPoint(0,cubicStrength), pos2+QPoint(0,cubicStrength), pos2);
    painter->drawPath(path);

 
}

void FloatingEdgeGraphic::setFloatPos(QPointF floatPos) {
    prepareGeometryChange();
    floatPos_ = floatPos;
    update();
}

