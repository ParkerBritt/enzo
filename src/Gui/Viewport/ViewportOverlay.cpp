#include "ViewportOverlay.h"
#include "Engine/Types.h"
#include "Gui/Viewport/ViewportCamerasModel.h"
#include "Engine/Operator/Camera.h"
#include "Engine/Operator/Primitive.h"
#include "Engine/Operator/Primitive.h"
#include <QAbstractItemView>
#include <memory>
#include <qboxlayout.h>
#include <QComboBox>
#include <QStyledItemDelegate>

ViewportOverlay::ViewportOverlay()
: QWidget()
{
    cameraDropdownModel_ = new ViewportCamerasModel();

    cameraDropdown_ = new QComboBox(this);
    cameraDropdown_->setStyleSheet(R"(
    QComboBox
    {
        background: rgba(0,0,0,0.2);
        border: 1px solid #303030;
        border-radius: 10px;
        padding: 3px 15px;;
    }

    QComboBox::drop-down {
        background: transparent;
        width: 0px;
    }

    QComboBox QAbstractItemView
    {
        background-color: rgba(0,0,0,0.2);
        border-radius: 8px;
        border: none;
        outline: none;
    }

    QComboBox QAbstractItemView::item
    {
        background: transparent;
        border: none;
        margin: 0px 5px;
        padding: 4px 5px;
        border-radius: 6px;
    }
    QComboBox QAbstractItemView::item:selected
    {
        margin: 5px;
        background: #282828;
        border: none;
    }

    )");
    cameraDropdown_->setModel(cameraDropdownModel_);
    cameraDropdown_->setItemDelegate(new QStyledItemDelegate(cameraDropdown_));

    connect(cameraDropdown_, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            // Index 0 is free camera, leaves the viewport camera alone.
            if (index <= 0) {
                selectedCameraPath_.clear();
                return;
            }
            if (!packet_) return;
            auto cams = packet_->getPrimitives(enzo::geo::PrimType::CAMERA);
            const int camIdx = index - 1;
            if (camIdx < 0 || camIdx >= static_cast<int>(cams.size())) return;
            selectedCameraPath_ = cams[camIdx]->getPath();
            Q_EMIT cameraSelected(
                std::static_pointer_cast<const enzo::geo::Camera>(cams[camIdx]));
        });

    topButtonsLayout_ = new QHBoxLayout();
    topButtonsLayout_->addStretch();
    topButtonsLayout_->addWidget(cameraDropdown_);

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
    // The model reset below resets the dropdown to index 0 (Free Cam) and fires
    // currentIndexChanged(0), which clears selectedCameraPath_. Save it first so
    // we can restore the user's selection against the new packet.
    const std::string savedPath = selectedCameraPath_;

    packet_ = packet;
    cameraDropdownModel_->setPacket(packet_);

    if (savedPath.empty() || !packet_) return;

    auto cams = packet_->getPrimitives(enzo::geo::PrimType::CAMERA);
    for (int i = 0; i < static_cast<int>(cams.size()); ++i) {
        if (cams[i]->getPath() == savedPath) {
            // Triggers the lambda above, which re-emits cameraSelected with the
            // (potentially updated) transform from the new packet.
            cameraDropdown_->setCurrentIndex(i + 1);
            return;
        }
    }
}

void ViewportOverlay::setFreeCam()
{
    cameraDropdown_->setCurrentIndex(0);
}

