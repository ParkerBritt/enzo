#include "Gui/Viewport/Viewport.h"
#include "Gui/Viewport/GLCamera.h"
#include <glm/common.hpp>
#include <qboxlayout.h>
#include <qevent.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <iostream>
#include <QTimer>
#include <QPainterPath>
#include <QEvent>

Viewport::Viewport(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    mainLayout_=new QVBoxLayout();
    openGLWidget_ = new ViewportGLWidget(this);
    mainLayout_->addWidget(openGLWidget_);

    this->setLayout(mainLayout_);
}

void Viewport::setGeometry(enzo::geo::Geometry& geometry)
{
    openGLWidget_->geometryChanged(geometry);
}

void Viewport::resizeEvent(QResizeEvent *event)
{
    QPainterPath path;
    constexpr float radius = 10;
    path.addRoundedRect(mainLayout_->contentsRect(), radius, radius);
    QRegion region = QRegion(path.toFillPolygon().toPolygon());
    this->setMask(region);
}

bool Viewport::event(QEvent *event)
{
    switch(event->type())
    {
        case QEvent::Wheel:
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            handleCamera(event);
            event->ignore();
            return true;
            break;
        default:
            return QWidget::event(event);
            break;
    }


}

void Viewport::handleCamera(QEvent *event)
{
    switch(event->type())
    {
        case QEvent::Wheel:
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            float delta = wheelEvent->angleDelta().y();
            constexpr float mouseSpeed = 0.7; 
            openGLWidget_->curCamera.changeRadius(-glm::sign(delta)*mouseSpeed);
            break;
        }
        case QEvent::MouseMove:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF mousePos = mouseEvent->position();
            GLCamera& camera = openGLWidget_->curCamera;
            constexpr float rotateSpeed = 0.01;
            constexpr float panSpeed = 0.01;
            constexpr float zoomSpeed = 0.01;

            if(leftMouseDown_)
            {
                QPointF delta = mousePos-leftStartPos_;
                delta*=-rotateSpeed;
                camera.rotateAroundCenter(delta.x(), {0,1,0});
                camera.rotateAroundCenter(delta.y(),
                    camera.getRight() * glm::vec3(1.0f,0.0f,1.0f));
                leftStartPos_=mousePos;
            }
            if(middleMouseDown_)
            {
                QPointF delta = mousePos-middleStartPos_;
                delta *= panSpeed;
                glm::vec3 up = camera.getUp()*static_cast<float>(-delta.y());
                glm::vec3 right = camera.getRight()*static_cast<float>(-delta.x());

                camera.changeCenter(up.x+right.x, up.y+right.y, up.z+right.z);
                camera.movePos(up.x+right.x, up.y+right.y, up.z+right.z);
                middleStartPos_=mousePos;
            }
            if(rightMouseDown_)
            {
                QPointF delta = mousePos-rightStartPos_;
                delta*=zoomSpeed;
                camera.changeRadius(-delta.x()+delta.y());
                rightStartPos_=mousePos;
            }
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        {
            bool isDown=event->type()==QEvent::MouseButtonPress;
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            switch(mouseEvent->button())
            {
                case Qt::LeftButton:
                    leftMouseDown_=isDown;
                    if(isDown) leftStartPos_=mouseEvent->position();
                    break;
                case Qt::MiddleButton:
                    middleMouseDown_=isDown;
                    if(isDown) middleStartPos_=mouseEvent->position();
                    break;
                case Qt::RightButton:
                    rightMouseDown_=isDown;
                    if(isDown) rightStartPos_=mouseEvent->position();
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}
