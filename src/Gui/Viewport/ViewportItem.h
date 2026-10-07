#pragma once
#include "Graphics/ViewportCamera.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <QColor>
#include <QQuickItem>
#include <QString>
#include <cstddef>
#include <optional>

namespace enzo::ui {

/// @brief The viewport surface in the QML scene, drawn by the graphics device.
class ViewportItem : public QQuickItem
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
        QColor geometryColor READ geometryColor WRITE setGeometryColor NOTIFY geometryColorChanged
    )
    Q_PROPERTY(
        bool wireframeVisible READ wireframeVisible WRITE setWireframeVisible NOTIFY
            wireframeVisibleChanged
    )
    Q_PROPERTY(bool pointsVisible READ pointsVisible WRITE setPointsVisible NOTIFY pointsVisibleChanged)
    Q_PROPERTY(
        QString viewCameraPath READ viewCameraPath WRITE setViewCameraPath NOTIFY viewCameraPathChanged
    )
  public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    ViewportViewModel* viewModel() const { return viewModel_; }
    void setViewModel(ViewportViewModel* viewModel);

    QColor backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const QColor& colour);

    /// @brief Returns the colour the display geometry is shaded with.
    QColor geometryColor() const { return geometryColor_; }
    void setGeometryColor(const QColor& colour);

    /// @brief Returns whether the wireframe is drawn over the display geometry.
    bool wireframeVisible() const { return wireframeVisible_; }
    void setWireframeVisible(bool visible);

    /// @brief Returns whether every mesh point is drawn, rather than only the points that belong
    /// to no face.
    bool pointsVisible() const { return pointsVisible_; }
    void setPointsVisible(bool visible);

    /// @brief Returns the path of the camera primitive the view looks through, or an empty path
    /// when it looks through the orbit camera.
    QString viewCameraPath() const { return viewCameraPath_; }
    void setViewCameraPath(const QString& path);

    /// @brief Turns the camera around its centre by a pointer drag.
    Q_INVOKABLE void orbit(qreal dx, qreal dy);

    /// @brief Slides the camera and its centre across the view by a pointer drag.
    Q_INVOKABLE void pan(qreal dx, qreal dy);

    /// @brief Moves the camera toward its centre for a negative amount and away for a positive one.
    Q_INVOKABLE void dolly(qreal amount);

  Q_SIGNALS:
    void viewModelChanged();
    void backgroundColorChanged();
    void geometryColorChanged();
    void wireframeVisibleChanged();
    void pointsVisibleChanged();
    void viewCameraPathChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  private:
    /// @brief Returns the index of the view camera in the geometry, or empty when the view looks
    /// through the orbit camera or the geometry holds no camera at that path.
    std::optional<std::size_t> getViewCameraIndex() const;

    /// @brief Moves the orbit camera to the view camera and looks through it instead.
    void detachFromViewCamera();

    /// @brief Looks through the orbit camera when the geometry no longer holds the view camera.
    void dropMissingViewCamera();

    ViewportViewModel* viewModel_ = nullptr;
    QColor backgroundColor_;
    QColor geometryColor_;
    bool wireframeVisible_ = true;
    bool pointsVisible_ = false;
    QString viewCameraPath_;
    gfx::ViewportCamera camera_;
};

} // namespace enzo::ui
