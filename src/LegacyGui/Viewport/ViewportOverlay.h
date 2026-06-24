#pragma once

#include "ViewportCamerasModel.h"
#include <Engine/Network/NodePacket.h>
#include <Engine/Primitives/Camera.h>
#include <QComboBox>
#include <QWidget>
#include <memory>
#include <qboxlayout.h>
#include <qwidget.h>
#include <string>

class ViewportOverlay : public QWidget
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
