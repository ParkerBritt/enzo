#pragma once

#include <QWidget>
#include <memory>
#include <qboxlayout.h>
#include <qwidget.h>
#include "ViewportCamerasModel.h"
#include <Engine/Operator/NodePacket.h>

class ViewportOverlay
: public QWidget
{
    public:
        ViewportOverlay();
        void setPacket(std::shared_ptr<const enzo::NodePacket> packet);
    private:
        QVBoxLayout* mainLayout_;
        QHBoxLayout* topButtonsLayout_;
        std::shared_ptr<const enzo::NodePacket> packet_;
        ViewportCamerasModel* cameraDropdownModel_;
};
