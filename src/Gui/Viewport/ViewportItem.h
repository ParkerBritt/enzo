#pragma once
#include "Engine/Network/NodePacket.h"
#include "Gui/Viewport/GLCamera.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <QColor>
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
    Q_PROPERTY(
        QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
            backgroundColorChanged
    )
    Q_PROPERTY(
        QColor gradientCenter READ gradientCenter WRITE setGradientCenter NOTIFY
            gradientCenterChanged
    )
    Q_PROPERTY(
        QColor gradientEdge READ gradientEdge WRITE setGradientEdge NOTIFY gradientEdgeChanged
    )
    Q_PROPERTY(
        QColor geometryColor READ geometryColor WRITE setGeometryColor NOTIFY geometryColorChanged
    )
  public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    Renderer* createRenderer() const override;

    ViewportViewModel* viewModel() const { return viewModel_; }
    void setViewModel(ViewportViewModel* viewModel);

    /// @brief Middle stop of the viewport background gradient.
    QColor backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const QColor& colour);

    /// @brief Bright centre stop of the background gradient.
    QColor gradientCenter() const { return gradientCenter_; }
    void setGradientCenter(const QColor& colour);

    /// @brief Dark edge stop of the background gradient.
    QColor gradientEdge() const { return gradientEdge_; }
    void setGradientEdge(const QColor& colour);

    /// @brief Flat shading colour of the display geometry.
    QColor geometryColor() const { return geometryColor_; }
    void setGeometryColor(const QColor& colour);

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
    void backgroundColorChanged();
    void gradientCenterChanged();
    void gradientEdgeChanged();
    void geometryColorChanged();

  private:
    GLCamera camera_{-10.f, 5.f, -10.f};
    ViewportViewModel* viewModel_ = nullptr;
    QColor backgroundColor_{"#101015"};
    QColor gradientCenter_{"#191920"};
    QColor gradientEdge_{"#0b0b0f"};
    QColor geometryColor_{"#a8a8b8"};

    // Geometry handed to the renderer on the next sync. Null once consumed.
    std::shared_ptr<const enzo::NodePacket> pendingGeometry_;
};

} // namespace enzo::ui
