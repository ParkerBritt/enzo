#include "gui/network/NetworkGraphicsScene.h"
#include <QGraphicsScene>
#include <QGraphicsSceneWheelEvent>
#include <iostream>
#include <qgraphicsview.h>

NetworkGraphicsScene::NetworkGraphicsScene()
: QGraphicsScene() 
{
    sceneWidth_ = 64000;
    sceneHeight_ = 64000;
    gridSize_ = 40;

    setSceneRect(sceneWidth_/-2.0f, sceneHeight_/-2.0f, sceneWidth_, sceneHeight_);
    
    setBackgroundBrush(QColor("#282828"));
}

void NetworkGraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);


    int top = ceil(rect.top());
    int bottom = floor(rect.bottom());
    int left = floor(rect.left());
    int right = ceil(rect.right());

    QPen gridPen(QColor("#323232"));

    painter->setPen(gridPen);

    QList<QLine> lines;

    for (int y = top - (top % gridSize_); y <= bottom; y += gridSize_) {
        lines.append(QLine(left, y, right, y));
    }

    for (int x = left - (left % gridSize_); x <= right; x += gridSize_) {
        lines.append(QLine(x, bottom, x, top));
    }

    painter->drawLines(lines);

}


