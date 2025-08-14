#include "Gui/Network/NetworkGraphicsView.h"
#include <QGraphicsItem>
#include "Gui/Network/Network.h"
#include <QCoreApplication>
#include "Gui/Network/NetworkGraphicsScene.h"
#include "Gui/Network/SocketGraphic.h"
#include <iostream>
#include <QMouseEvent>
#include <QScrollBar>
#include <qgraphicsitem.h>
#include <qobject.h>
#include <typeinfo>

NetworkGraphicsView::NetworkGraphicsView(QWidget *parent, Network* network, QGraphicsScene* scene)
: QGraphicsView(parent), scene_{scene}, network_{network}
{
    setScene(scene_);
    setMouseTracking(true);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    initUI();


}

void NetworkGraphicsView::initUI()
{
    // zoom from mouse
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // disable scroll bars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    //
    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
}

void NetworkGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);

    // pass event to parent
    if (parentWidget()) {
       QMouseEvent *eventCopy = new QMouseEvent(
            event->type(),
            event->position(),
            event->scenePosition(),
            event->globalPosition(),
            event->button(),
            event->buttons(),
            event->modifiers()
        );
        QCoreApplication::postEvent(parentWidget(), eventCopy);
    }
}

void NetworkGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if(
        event->button() == Qt::MiddleButton
    )
    {
        panStartPos = event->pos();
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);

    // pass event to parent
    // if (parentWidget()) {
    //    QMouseEvent *eventCopy = new QMouseEvent(
    //         event->type(),
    //         event->position(),
    //         event->scenePosition(),
    //         event->globalPosition(),
    //         event->button(),
    //         event->buttons(),
    //         event->modifiers()
    //     );
    //     QCoreApplication::postEvent(parentWidget(), eventCopy);
    // }
}



// void NetworkView::mouseReleaseEvent(QMouseEvent *event)
// {
//     if(
//         event->button() & Qt::MiddleButton
//     )
//     {
//         panStartPos = QPointF(0,0);
//     }
// }

void NetworkGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    network_->mouseMoved(event);

    // pan view
    if( event->buttons() & Qt::MiddleButton)
    {
        QPointF pos = event->pos();
        QPointF delta = pos-panStartPos;

        float speed = 1.0f;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        panStartPos = event->pos();
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
    
}


void NetworkGraphicsView::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if(delta > 0)
    {
        scale(1.1, 1.1);
    }
    else if(delta < 0)
    {
        scale(0.9, 0.9);
    }
    event->accept();
}

