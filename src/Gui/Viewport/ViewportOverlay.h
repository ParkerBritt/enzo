#pragma once

#include <QWidget>
#include <QComboBox>
#include <memory>
#include <string>
#include <qboxlayout.h>
#include <qwidget.h>
#include "ViewportCamerasModel.h"
#include <Engine/Operator/NodePacket.h>
#include <Engine/Operator/Camera.h>

class ViewportOverlay
: public QWidget
{
    Q_OBJECT
    public:
        ViewportOverlay();
        void setPacket(std::shared_ptr<const enzo::NodePacket> packet);
        void setFreeCam();
    Q_SIGNALS:
        void cameraSelected(std::shared_ptr<const enzo::geo::Camera> camera);
    private:
        QVBoxLayout* mainLayout_;
        QHBoxLayout* topButtonsLayout_;
        std::shared_ptr<const enzo::NodePacket> packet_;
        ViewportCamerasModel* cameraDropdownModel_;
        QComboBox* cameraDropdown_;
        std::string selectedCameraPath_;
};
