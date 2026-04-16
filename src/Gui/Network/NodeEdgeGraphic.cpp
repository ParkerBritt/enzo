#include "Gui/Network/NodeEdgeGraphic.h"
#include <QTextDocument>
#include "Engine/Operator/GeometryConnection.h"
#include "Gui/Network/SocketGraphic.h"
#include <iostream>
#include <qgraphicsitem.h>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include "icecream.hpp"

NodeEdgeGraphic::NodeEdgeGraphic(SocketGraphic* socket1, SocketGraphic* socket2, std::weak_ptr<enzo::nt::GeometryConnection> connection, QGraphicsItem *parent)
: QGraphicsItem(parent), socket1_{socket1}, socket2_{socket2}, defaultColor_{QColor("white")}, connection_{connection}
{
    setAcceptHoverEvents(true);

    setZValue(-1);
    color_=defaultColor_;
    defaultPen_=QPen(defaultColor_);
    defaultPen_.setCapStyle(Qt::RoundCap);

    deleteHighlightPen_.setCapStyle(Qt::RoundCap);
    deleteHighlightPen_.setWidth(2);
    updateDeleteHighlightPen();

    // remove self when connection is removed
    if(auto connectionShared = connection_.lock())
    {
        connectionShared->removed.connect([this](){ this->remove(false); });
    }

    socket1_->addEdge(this);
    socket2_->addEdge(this);
}

void NodeEdgeGraphic::updateDeleteHighlightPen()
{
    QLinearGradient gradient(pos1_, hoverPos_);
    gradient.setColorAt(0.0, QColor(255, 74, 74, 200));
    gradient.setColorAt(1, QColor(255, 74, 74, 50));
    // gradient.setColorAt(1.0, QColor(255, 74, 74, 200));

    deleteHighlightPen_.setBrush(QBrush(gradient));
}

NodeEdgeGraphic::~NodeEdgeGraphic()
{
    std::cout << "edge destructor\n";
    remove();
    std::cout << "destructor finished\n";
}

void NodeEdgeGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    hoverPos_ = event->scenePos();
    if(deleteHighlight_)
    {
        updateDeleteHighlightPen();
        update();
    }
}

void NodeEdgeGraphic::updatePath()
{
    qreal cubicStrength = 40;
    path_.clear();
    path_.moveTo(pos1_);
    path_.cubicTo(pos1_-QPoint(0,cubicStrength), pos2_+QPoint(0,cubicStrength), pos2_);
    updateDeleteHighlightPen();
    update();

}

void NodeEdgeGraphic::setPos(QPointF pos1, QPointF pos2)
{
    prepareGeometryChange();
    pos1_ = pos1;
    pos2_ = pos2;
    updatePath();
}

void NodeEdgeGraphic::setDeleteHighlight(bool state)
{
    bool stateChanged=deleteHighlight_!=state;
    deleteHighlight_=state;

    if(stateChanged)
    {
        update();
    }
}

void NodeEdgeGraphic::setStartPos(QPointF pos)
{
    prepareGeometryChange();
    pos1_ = pos;
    updatePath();
}

void NodeEdgeGraphic::setEndPos(QPointF pos)
{
    prepareGeometryChange();
    pos2_ = pos;
    updatePath();
}

QRectF NodeEdgeGraphic::boundingRect() const
{
    // std::cout << "bounds set" << socket1_->scenePos().x() << " " << socket1_->scenePos().y() << " " << socket2_->scenePos().x() << " " << socket2_->scenePos().y() << "\n";
    // QRectF boundRect_ = QRectF(socket1_->scenePos(), socket1_->scenePos()).normalized();
    QRectF boundRect_ = QRectF(pos1_, pos2_).normalized();
    boundRect_.adjust(-padding_,-padding_,padding_,padding_);
    return boundRect_;
}

QPainterPath NodeEdgeGraphic::shape() const{
    QPainterPathStroker stroker;
    stroker.setWidth(padding_);
    return stroker.createStroke(path_);
}

QPointF NodeEdgeGraphic::closestPoint(QPointF startPos)
{
    QPointF lineVec = pos2_ - pos1_;
    QPointF pointVec = startPos - pos1_;

    double lineLenSquared = QPointF::dotProduct(lineVec, lineVec);
    if (lineLenSquared == 0.0)
        return pos1_;

    double t = QPointF::dotProduct(pointVec, lineVec) / lineLenSquared;

    QPointF closest = pos1_ + t * lineVec;
    return closest;
}


// void NodeEdgeGraphic::setColor(QColor color)
// {
//     std::cout << "color set to: " << color.name().toStdString() << "\n";
//     color_ = color;
//     pen_.setColor(color_);
//     update();
// }

// void NodeEdgeGraphic::useDefaultColor()
// {
//     setColor(defaultColor_);
// }

void NodeEdgeGraphic::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // std::cout << "painting\n";
    painter->setPen(deleteHighlight_ ? deleteHighlightPen_ : defaultPen_);
    painter->drawPath(path_);
 
}

void NodeEdgeGraphic::remove(bool full)
{
    if(removed_)
    {
        std::cerr << "WARNING: avoiding double edge graphic deletion\n";
    }
    removed_ = true;
    IC();
    // TODO: possible memory leak
    // these probably aren't necessary but i'm trying to fix a bug

    IC();
    if(connection_.expired())
    {
        return;
    }
    IC();

    IC();
    // prepareGeometryChange();
    IC();
    // update();
    IC();
    // scene()->update();
    IC();

    IC();
    scene()->removeItem(this);
    IC();
    socket1_->removeEdge(this);
    IC();
    socket2_->removeEdge(this);
    IC();

    if(full)
    {
        if(auto connectionShared = connection_.lock())
        {
            // remove connection
            connection_ = std::weak_ptr<enzo::nt::GeometryConnection>();
            connectionShared->remove();
        }
    }
    IC();
}
