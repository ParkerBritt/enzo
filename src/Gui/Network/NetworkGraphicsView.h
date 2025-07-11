#pragma once
#include <qgraphicsitem.h>
#include <qwidget.h>
#include <QGraphicsView>
#include <QGraphicsScene>

class Network;

class NetworkGraphicsView
: public QGraphicsView
{
public:
    NetworkGraphicsView(QWidget *parent = nullptr, Network* network=nullptr, QGraphicsScene* scene = nullptr);
    QSize sizeHint() const override { return QSize(-1, -1); }
private:
    QPointF panStartPos;
    void initUI();
    QGraphicsScene* scene_;
    Network* network_;

protected:
    void mouseMoveEvent(QMouseEvent *mouseEvent) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    QGraphicsItem* getItemAtClick(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;

};
