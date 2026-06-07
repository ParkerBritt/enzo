#pragma once

#include <QPointF>
#include <QRectF>
#include <QWidget>
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
  public:
    Ramp(QWidget* parent = nullptr);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    /// A single ramp knot ordered by pointNumber with an x position and a y value.
    struct ControlPoint
    {
        int pointNumber;
        double position; // x axis between 0 and 1
        double value;    // y axis unconstrained
    };

    /// Which handle of a control point a drag is acting on.
    enum class Handle
    {
        None,
        Circle,
        Square
    };

    /// How the curve is interpolated between control points.
    enum class Interpolation
    {
        Linear,
        Constant,      // not yet implemented
        BSpline,       // not yet implemented
        CatmullRom,    // not yet implemented
        MonotoneCubic, // not yet implemented
        Bezier,        // not yet implemented
        Hermite        // not yet implemented
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

    int circleHitIndex_(const QPointF& pos) const;
    int squareHitIndex_(const QPointF& pos) const;

    std::vector<ControlPoint> controlPoints_;

    int activePoint_ = -1;
    Handle activeHandle_ = Handle::None;
    Interpolation interpolation_ = Interpolation::Linear;

    // Visible value range mapped across the panel height
    double valueMin_ = 0.0;
    double valueMax_ = 1.0;
};

} // namespace enzo::ui
