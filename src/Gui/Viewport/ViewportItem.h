#pragma once
#include "Graphics/ViewportCamera.h"
#include <QColor>
#include <QQuickItem>

namespace enzo::ui {

/// @brief The viewport surface in the QML scene, drawn by the graphics device.
class ViewportItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
            backgroundColorChanged
    )
  public:
    explicit ViewportItem(QQuickItem* parent = nullptr);

    QColor backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const QColor& colour);

    /// @brief Turns the camera around its centre by a pointer drag.
    Q_INVOKABLE void orbit(qreal dx, qreal dy);

    /// @brief Slides the camera and its centre across the view by a pointer drag.
    Q_INVOKABLE void pan(qreal dx, qreal dy);

    /// @brief Moves the camera toward its centre for a negative amount and away for a positive one.
    Q_INVOKABLE void dolly(qreal amount);

  Q_SIGNALS:
    void backgroundColorChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  private:
    QColor backgroundColor_;
    gfx::ViewportCamera camera_;
};

} // namespace enzo::ui
