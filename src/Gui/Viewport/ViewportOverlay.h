#pragma once

#include <QWidget>
#include <qboxlayout.h>
#include <qwidget.h>
#include "ViewportCamerasModel.h"
#include <Engine/Operator/NodePacket.h>

class ViewportOverlay
: public QWidget
{
    public:
        ViewportOverlay();
        void setPacket(enzo::NodePacket& packet);
    private:
        QVBoxLayout* mainLayout_;
        QHBoxLayout* topButtonsLayout_;
        enzo::NodePacket packet_;
        ViewportCamerasModel* cameraDropdownModel_;
};
