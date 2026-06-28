#pragma once
#include "Engine/Network/NodePacket.h"
#include "Gui/Viewport/GLCamera.h"
#include <QQuickFramebufferObject>
#include <memory>

namespace enzo::ui
{

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
  public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    /// @brief Camera state read by the renderer each sync.
    const GLCamera& getCamera() const { return camera_; }

    /// @brief Geometry the renderer uploads on its next sync, if any.
    std::shared_ptr<const enzo::NodePacket> takePendingGeometry();

    /// @brief Shows a new packet and schedules a repaint.
    void setGeometry(std::shared_ptr<const enzo::NodePacket> packet);

  private:
    GLCamera camera_{-10.f, 5.f, -10.f};

    // Geometry handed to the renderer on the next sync. Null once consumed.
    std::shared_ptr<const enzo::NodePacket> pendingGeometry_;
};

} // namespace enzo::ui
