#include "Gui/Network/SocketGraphic.h"
#include "Gui/Network/NodeEdgeGraphic.h"
#include "icecream.hpp"
#include <QPropertyAnimation>
#include <QTextDocument>
#include <iostream>
#include <qgraphicsitem.h>

namespace {

// Lower is faster
constexpr int hoverDurationMs = 200;
// Size of a socket while a connection hovers it
constexpr qreal hoverScale = 1.2;

} // namespace

SocketGraphic::SocketGraphic(
    enzo::nt::SocketIOType type,
    enzo::nt::OpId opId,
    unsigned int socketIndex,
    QGraphicsItem* parent
)
    : QGraphicsObject(parent), type_{type}, opId_{opId}, socketIndex_{socketIndex}
{
    brushActive_ = QBrush("white");
    brushInactive_ = QBrush("#9f9f9f");
    socketSize_ = 3;
    initBoundingBox();

    scaleAnim_ = new QPropertyAnimation(this, "scale", this);
    scaleAnim_->setDuration(hoverDurationMs);
    // Pop out past the target before settling
    scaleAnim_->setEasingCurve(QEasingCurve::OutBack);
}

unsigned int SocketGraphic::getIndex() const { return socketIndex_; }

void SocketGraphic::addEdge(NodeEdgeGraphic* edge)
{
    // std::cout << "adding edge\n";
    // const bool isInput = getIO()==enzo::nt::SocketIOType::Input;
    // if(isInput && edges_.size()>0)
    // {
    //     std::unordered_set<NodeEdgeGraphic*> edgesCopy = edges_;
    //     for(NodeEdgeGraphic* edge : edgesCopy)
    //     {
    //         if(!edge)
    //         {
    //             continue;
    //         }
    //         edge->remove();
    //     }
    //     edges_.clear();
    // }
    edges_.insert(edge);
}

void SocketGraphic::removeEdge(NodeEdgeGraphic* edge)
{
    std::cout << "removing edge\n";
    edges_.erase(edge);
    // auto it = find(edges_.begin(), edges_.end(), edge);
    // if(it!=edges_.end())
    // {
    //     edges_.erase(it);
    // }
}

void SocketGraphic::initBoundingBox()
{
    boundRect_ = QRect(
        -socketSize_ / 2.0f * paddingScale_,
        -socketSize_ / 2.0f * paddingScale_,
        socketSize_ * paddingScale_,
        socketSize_ * paddingScale_
    );
}

void SocketGraphic::posChanged(QPointF pos)
{
    for (auto* edge : edges_)
    {
        // edge->setPos(startSocket_->scenePos(), socket->scenePos());
        if (type_ == enzo::nt::SocketIOType::Input)
        {
            edge->setStartPos(pos);
        }
        else if (type_ == enzo::nt::SocketIOType::Output)
        {
            edge->setEndPos(pos);
        }
    }
}

enzo::nt::OpId SocketGraphic::getOpId() const { return opId_; }

QRectF SocketGraphic::boundingRect() const { return boundRect_; }

QPainterPath SocketGraphic::shape() const
{
    QPainterPath path;
    QPointF startPt(
        boundRect_.center().x(),
        type_ == enzo::nt::SocketIOType::Input ? boundRect_.top() : boundRect_.bottom()
    );
    path.moveTo(startPt);
    path.arcTo(boundRect_, 0, type_ == enzo::nt::SocketIOType::Input ? 180 : -180);
    path.lineTo(boundRect_.right(), boundRect_.center().y());
    path.closeSubpath();

    return path;
}

enzo::nt::SocketIOType SocketGraphic::getIO() { return type_; }

void SocketGraphic::paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget
)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(hovered_ ? brushActive_ : brushInactive_);
    painter->drawEllipse(QPoint(0, 0), socketSize_, socketSize_);

    // painter->drawRect(boundRect_);
}

void SocketGraphic::setHover(bool state)
{
    if (state == hovered_) return;

    hovered_ = state;
    animateScale(state ? hoverScale : 1.0);
    update();
}

void SocketGraphic::animateScale(qreal target)
{
    // Pop toward the target size, restarting from wherever the socket currently sits
    scaleAnim_->stop();
    scaleAnim_->setStartValue(scale());
    scaleAnim_->setEndValue(target);
    scaleAnim_->start();
}

void SocketGraphic::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    hovered_ = true;
    update();
}

void SocketGraphic::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    hovered_ = false;
    update();
}
