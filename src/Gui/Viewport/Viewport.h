#pragma once
#include <qboxlayout.h>
#include <qwidget.h>
#include "Gui/Viewport/ViewportGLWidget.h"

class Viewport
: public QWidget
{
public:
    Viewport(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    void setGeometry(enzo::geo::Geometry& geometry);
private:
    QVBoxLayout* mainLayout_;
    ViewportGLWidget* openGLWidget_;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    Qt::Key cameraMod_ = Qt::Key_Space;
    void handleCamera(QEvent *event);

    // TODO: maybe simplify positions to mouseDownPos
    bool middleMouseDown_=false;
    QPointF middleStartPos_;
    bool leftMouseDown_=false;
    QPointF leftStartPos_;
    bool rightMouseDown_=false;
    QPointF rightStartPos_;
};
