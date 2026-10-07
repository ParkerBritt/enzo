#pragma once
#include "Graphics/ViewportCamera.h"
#include "Gui/Viewport/ViewportViewModel.h"
#include <QColor>
#include <QQuickItem>

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
  public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    ViewportViewModel* viewModel() const { return viewModel_; }
    void setViewModel(ViewportViewModel* viewModel);

    QColor backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const QColor& colour);

    /// @brief Returns the colour the display geometry is shaded with.
    QColor geometryColor() const { return geometryColor_; }
    void setGeometryColor(const QColor& colour);

    /// @brief Turns the wireframe over the display geometry on or off.
    Q_INVOKABLE void toggleWireframe();

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

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  private:
    ViewportViewModel* viewModel_ = nullptr;
    QColor backgroundColor_;
    QColor geometryColor_;
    bool wireframeVisible_ = true;
    gfx::ViewportCamera camera_;
};

} // namespace enzo::ui
