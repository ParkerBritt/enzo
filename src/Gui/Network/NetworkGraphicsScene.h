#pragma once

#include <QGraphicsScene>

class NetworkGraphicsScene
: public QGraphicsScene
{
public:
    NetworkGraphicsScene();
private:
    unsigned int sceneWidth_;
    unsigned int sceneHeight_;
    unsigned int gridSize_;
protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
};
