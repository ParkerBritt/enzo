#pragma once
#include "Engine/Core/Types.h"
#include <QGraphicsObject>
#include <QPainter>
#include <unordered_set>

class NodeEdgeGraphic;
class QPropertyAnimation;

class SocketGraphic : public QGraphicsObject
{
    Q_OBJECT

  public:
    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    SocketGraphic(
        enzo::nt::SocketIOType type,
        enzo::nt::OpId opId,
        unsigned int socketIndex,
        QGraphicsItem* parent = nullptr
    );
    enzo::nt::SocketIOType getIO();
    void addEdge(NodeEdgeGraphic* edge);
    void removeEdge(NodeEdgeGraphic* edge);
    void posChanged(QPointF pos);
    QPainterPath shape() const override;
    enzo::nt::OpId getOpId() const;

    unsigned int getIndex() const;
    void setHover(bool state);

  private:
    int socketSize_ = 1;
    unsigned int socketIndex_;
    QBrush brushInactive_;
    QBrush brushActive_;
    bool hovered_ = false;
    enzo::nt::SocketIOType type_;
    std::unordered_set<NodeEdgeGraphic*> edges_;
    qreal paddingScale_ = 20;
    QRectF boundRect_;
    enzo::nt::OpId opId_;
    QPropertyAnimation* scaleAnim_;

    void initBoundingBox();
    void animateScale(qreal target);

  protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
};
