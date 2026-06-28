#pragma once
#include "Engine/Network/NodePacket.h"
#include "Gui/Viewport/GLCamera.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <QQuickFramebufferObject>
#include <memory>

namespace enzo::ui {

/// @brief Viewport surface that draws the display geometry with OpenGL.
///
/// A `QQuickFramebufferObject` renders into an offscreen buffer that the Quick
/// scene graph composites as an ordinary item, so the GL viewport lives inside
/// the QML layout. The item owns the camera and the geometry to show, while its
/// renderer owns the GL resources on the render thread.
///
/// @note Placeholder until the viewport is rebuilt on the Diligent engine.
class ViewportItem : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        enzo::ui::ViewportViewModel* viewModel READ viewModel WRITE setViewModel NOTIFY
            viewModelChanged
    )
  public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    ViewportViewModel* viewModel() const { return viewModel_; }
    void setViewModel(ViewportViewModel* viewModel);

    /// @brief Orbits the camera around the centre by a pointer drag.
    Q_INVOKABLE void orbit(qreal dx, qreal dy);

    /// @brief Slides the camera and its centre across the view plane.
    Q_INVOKABLE void pan(qreal dx, qreal dy);

    /// @brief Dollies the camera toward or away from the centre.
    Q_INVOKABLE void zoom(qreal amount);

    /// @brief Camera state read by the renderer each sync.
    const GLCamera& getCamera() const { return camera_; }

    /// @brief Geometry the renderer uploads on its next sync, if any.
    std::shared_ptr<const enzo::NodePacket> takePendingGeometry();

    /// @brief Shows a new packet and schedules a repaint.
    void setGeometry(std::shared_ptr<const enzo::NodePacket> packet);

  Q_SIGNALS:
    void viewModelChanged();

  private:
    GLCamera camera_{-10.f, 5.f, -10.f};
    ViewportViewModel* viewModel_ = nullptr;

    // Geometry handed to the renderer on the next sync. Null once consumed.
    std::shared_ptr<const enzo::NodePacket> pendingGeometry_;
};

} // namespace enzo::ui
