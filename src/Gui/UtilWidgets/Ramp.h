#pragma once

#include "Engine/Parameter/Ramp.h"
#include <QEasingCurve>
#include <QPointF>
#include <QRectF>
#include <QWidget>
#include <algorithm>
#include <qtmetamacros.h>
#include <vector>

class QPaintEvent;
class QMouseEvent;

namespace enzo::ui {

/**
 * @brief Ramp editor that draws a curve across a framed panel and emits changes
 * as control points are edited.
 */
class Ramp : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverScale READ hoverScale WRITE setHoverScale)
  public:
    Ramp(QWidget* parent = nullptr);

    // Control point handle sizes, shared with the layout so the panel inset can
    // be sized to exactly clear them.
    static constexpr double circleRadius = 5.0;
    static constexpr double squareSize = 9.0;
    static constexpr double hoverScaleTarget = 1.15;

    // Inset from the widget edge to the curve panel, just wide enough that the
    // largest handle a point can draw stays inside the widget.
    static constexpr double panelInset =
        std::max(circleRadius * hoverScaleTarget, squareSize / 2.0);

    /// @brief Replaces the displayed control points, one per QPointF where x is
    /// position and y is value. The interp of each point governs the segment to
    /// its right. Fits the value axis to span the points.
    void
    setPoints(const std::vector<QPointF>& points, const std::vector<prm::Interpolation>& interps);

    /// @brief Marks which control point is the active one for the host editor.
    void setSelectedPoint(int pointIndex);

    /// @brief The current control points, one per QPointF where x is position
    /// and y is value, ordered by position.
    std::vector<QPointF> points() const;

    /// @brief The interp of each control point, ordered by position to match
    /// points(). Each entry governs the segment toward the next point.
    std::vector<prm::Interpolation> interps() const;

  Q_SIGNALS:
    /// @brief Emitted whenever a drag or click changes the control points.
    void edited();

    /// @brief Emitted as a press begins an edit, before the points change.
    void editBegan();

    /// @brief Emitted as the press releases and the edit settles.
    void editEnded();

    /// @brief Emitted when interaction grabs or creates a different control point.
    void selectionChanged(int pointIndex);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    /// A single ramp knot ordered by pointNumber with an x position and a y value.
    struct ControlPoint
    {
        int pointNumber;
        double position; // x axis between 0 and 1
        double value;    // y axis unconstrained
        // Governs how the segment toward the next point is drawn.
        prm::Interpolation interp = prm::Interpolation::LINEAR;
    };

    /// Which handle of a control point a drag is acting on.
    enum class Handle
    {
        None,
        Circle,
        Square
    };

    QRectF backgroundRect_() const;
    double positionToX_(double position) const;
    double valueToY_(double value) const;
    double xToPosition_(double x) const;
    double yToValue_(double y) const;

    std::vector<QPointF> buildCurveTop_() const;
    void paintCurve_(QPainter& painter) const;
    void paintControlPoints_(QPainter& painter) const;
    void addControlPoint_(double position, double value);
    void selectPoint_(int pointIndex);
    void sortAndRenumber_();

    int circleHitIndex_(const QPointF& pos) const;
    int squareHitIndex_(const QPointF& pos) const;

    qreal hoverScale() const { return hoverScale_; }
    void setHoverScale(qreal scale)
    {
        hoverScale_ = scale;
        update();
    }
    void animateHoverScale_(qreal target, QEasingCurve::Type easing);
    void hoverPoint_(int pointIndex);

    std::vector<ControlPoint> controlPoints_;
    int selectedPoint_ = -1;

    // The dot the hover scale currently applies to, with its animated scale and target
    int scaledPoint_ = -1;
    qreal hoverScale_ = 1.0;
    qreal scaleTarget_ = 1.0;

    int activePoint_ = -1;
    Handle activeHandle_ = Handle::None;

    // Visible value range mapped across the panel height
    double valueMin_ = 0.0;
    double valueMax_ = 1.0;
};

} // namespace enzo::ui
