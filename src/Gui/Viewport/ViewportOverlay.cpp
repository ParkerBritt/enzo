#include "ViewportOverlay.h"
#include "Gui/Viewport/ViewportCamerasModel.h"
#include <qboxlayout.h>
#include <QComboBox>

ViewportOverlay::ViewportOverlay()
: QWidget()
{
    cameraDropdownModel_ = new ViewportCamerasModel();

    QComboBox* button = new QComboBox();
    button->setModel(cameraDropdownModel_);

    topButtonsLayout_ = new QHBoxLayout();
    topButtonsLayout_->addStretch();
    topButtonsLayout_->addWidget(button);

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->addLayout(topButtonsLayout_);
    mainLayout_->addStretch();

    setProperty("class", "ViewportOverlay");
    setStyleSheet(R"(
    .ViewportOverlay
    {
        background: transparent; 
    }
    )");
}

void ViewportOverlay::setPacket(std::shared_ptr<const enzo::NodePacket> packet)
{
    packet_ = packet;
    cameraDropdownModel_->setPacket(packet_);
}

