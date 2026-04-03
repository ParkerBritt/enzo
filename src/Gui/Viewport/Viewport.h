#pragma once
#include <qboxlayout.h>
#include <qwidget.h>
#include "Gui/Viewport/ViewportGLWidget.h"
#include "Gui/Panels/Panel.h"

class Viewport
: public Panel
{
public:
    Viewport(QWidget *parent = nullptr);
    void setGeometry(enzo::geo::Primitive& geometry);
    void clearGeometry();
private:
    QVBoxLayout* mainLayout_;
    ViewportGLWidget* openGLWidget_;
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
