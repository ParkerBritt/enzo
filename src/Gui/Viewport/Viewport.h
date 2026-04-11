#pragma once
#include <memory>
#include <qboxlayout.h>
#include <qwidget.h>
#include "Gui/Viewport/ViewportGLWidget.h"
#include "Gui/Panels/Panel.h"
#include "Engine/Operator/NodePacket.h"
#include "Gui/Viewport/ViewportOverlay.h"
#include <QStackedLayout>

class Viewport
: public Panel
{
public:
    Viewport(QWidget *parent = nullptr);
    void setGeometry(std::shared_ptr<const enzo::NodePacket> packet);
    void clearGeometry();
private:
    QStackedLayout* mainLayout_;
    ViewportGLWidget* openGLWidget_;
    ViewportOverlay* overlay_;
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
