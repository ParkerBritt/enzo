#include "Gui/UtilWidgets/Ramp.h"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace {
// Inset from the widget edge to the background panel so handles can overflow it
constexpr double padding = 12.0;
constexpr double cornerRadius = 9.0;
constexpr double circleRadius = 5.0;
constexpr double squareSize = 9.0;
const QColor panelColor("#1a1a1a");
const QColor borderColor("#383838");
const QColor circleColor("#ffffff");
const QColor squareColor("#B1B2B5");
const QColor curveStrokeColor("#B1B2B5");
const QColor curveFillColor(177, 178, 181, 50);
const QColor selectedRingColor("#4a90d9");
} // namespace

enzo::ui::Ramp::Ramp(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(100);

    // Default endpoints rising from zero to one
    controlPoints_.push_back({0, 0.0, 0.0});
    controlPoints_.push_back({1, 1.0, 1.0});
}

void enzo::ui::Ramp::setPoints(const std::vector<QPointF>& points)
{
    controlPoints_.clear();
    controlPoints_.reserve(points.size());
    for (const QPointF& point : points)
        controlPoints_.push_back({0, point.x(), point.y()});
    sortAndRenumber_();

    // Fit the value axis so every point sits inside the panel, always keeping
    // the zero to one band visible as a reference.
    double minValue = 0.0;
    double maxValue = 1.0;
    for (const ControlPoint& controlPoint : controlPoints_)
    {
        minValue = std::min(minValue, controlPoint.value);
        maxValue = std::max(maxValue, controlPoint.value);
    }
    valueMin_ = minValue;
    valueMax_ = maxValue;

    update();
}

void enzo::ui::Ramp::setSelectedPoint(int pointIndex)
{
    selectedPoint_ = pointIndex;
    update();
}

std::vector<QPointF> enzo::ui::Ramp::points() const
{
    std::vector<QPointF> result;
    result.reserve(controlPoints_.size());
    for (const ControlPoint& controlPoint : controlPoints_)
        result.push_back(QPointF(controlPoint.position, controlPoint.value));
    return result;
}

QRectF enzo::ui::Ramp::backgroundRect_() const
{
    return QRectF(rect()).adjusted(padding, padding, -padding, -padding);
}

double enzo::ui::Ramp::positionToX_(double position) const
{
    const QRectF panel = backgroundRect_();
    return panel.left() + panel.width() * position;
}

double enzo::ui::Ramp::valueToY_(double value) const
{
    const QRectF panel = backgroundRect_();
    const double normalized = (value - valueMin_) / (valueMax_ - valueMin_);
    return panel.bottom() - panel.height() * normalized;
}

double enzo::ui::Ramp::xToPosition_(double x) const
{
    const QRectF panel = backgroundRect_();
    return std::clamp((x - panel.left()) / panel.width(), 0.0, 1.0);
}

double enzo::ui::Ramp::yToValue_(double y) const
{
    const QRectF panel = backgroundRect_();
    const double normalized = (panel.bottom() - y) / panel.height();
    return valueMin_ + (valueMax_ - valueMin_) * normalized;
}

void enzo::ui::Ramp::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Rounded background panel inset from the widget so points can sit on its edges
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(panelColor);
    painter.drawRoundedRect(backgroundRect_(), cornerRadius, cornerRadius);

    paintCurve_(painter);
    paintControlPoints_(painter);
}

std::vector<QPointF> enzo::ui::Ramp::buildCurveTop_() const
{
    std::vector<QPointF> topPoints;
    if (controlPoints_.empty()) return topPoints;

    const QRectF panel = backgroundRect_();

    switch (interpolation_)
    {
    case Interpolation::Linear:
    default:
        // Flat extension from the left edge to the first control point
        topPoints.push_back(QPointF(panel.left(), valueToY_(controlPoints_.front().value)));
        for (const ControlPoint& controlPoint : controlPoints_)
            topPoints.push_back(
                QPointF(positionToX_(controlPoint.position), valueToY_(controlPoint.value))
            );
        // Flat extension from the last control point to the right edge
        topPoints.push_back(QPointF(panel.right(), valueToY_(controlPoints_.back().value)));
        break;
    }
    return topPoints;
}

void enzo::ui::Ramp::paintCurve_(QPainter& painter) const
{
    if (controlPoints_.size() < 2) return;

    const std::vector<QPointF> topPoints = buildCurveTop_();
    const QRectF panel = backgroundRect_();

    // Fill drops from the curve down to the panel bottom
    QPainterPath fillPath;
    fillPath.moveTo(topPoints.front().x(), panel.bottom());
    for (const QPointF& topPoint : topPoints)
        fillPath.lineTo(topPoint);
    fillPath.lineTo(topPoints.back().x(), panel.bottom());
    fillPath.closeSubpath();

    // Stroke runs only along the top through the control points
    QPainterPath strokePath;
    strokePath.moveTo(topPoints.front());
    for (std::size_t pointIndex = 1; pointIndex < topPoints.size(); ++pointIndex)
        strokePath.lineTo(topPoints[pointIndex]);

    // Rounded panel clip keeps the fill clear of the corners
    QPainterPath panelClip;
    panelClip.addRoundedRect(panel, cornerRadius, cornerRadius);

    painter.save();
    painter.setClipPath(panelClip);

    painter.setPen(Qt::NoPen);
    painter.setBrush(curveFillColor);
    painter.drawPath(fillPath);

    painter.setPen(QPen(curveStrokeColor, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(strokePath);

    painter.restore();
}

void enzo::ui::Ramp::paintControlPoints_(QPainter& painter) const
{
    const double squareCenterY = backgroundRect_().bottom();

    for (int pointIndex = 0; pointIndex < static_cast<int>(controlPoints_.size()); ++pointIndex)
    {
        const ControlPoint& controlPoint = controlPoints_[pointIndex];
        const double centerX = positionToX_(controlPoint.position);

        // Square centered on the background bottom edge sharing the control point x
        const QRectF squareRect(
            centerX - squareSize / 2.0,
            squareCenterY - squareSize / 2.0,
            squareSize,
            squareSize
        );
        painter.setPen(Qt::NoPen);
        painter.setBrush(squareColor);
        painter.drawRect(squareRect);

        // Free moving circle at the control point value, ringed when selected
        const double circleCenterY = valueToY_(controlPoint.value);
        painter.setBrush(circleColor);
        painter.setPen(pointIndex == selectedPoint_ ? QPen(selectedRingColor, 2) : QPen(Qt::NoPen));
        painter.drawEllipse(QPointF(centerX, circleCenterY), circleRadius, circleRadius);
        painter.setPen(Qt::NoPen);
    }
}

int enzo::ui::Ramp::circleHitIndex_(const QPointF& pos) const
{
    for (int pointIndex = 0; pointIndex < static_cast<int>(controlPoints_.size()); ++pointIndex)
    {
        const ControlPoint& controlPoint = controlPoints_[pointIndex];
        const QPointF center(positionToX_(controlPoint.position), valueToY_(controlPoint.value));
        // Hit radius runs larger than the drawn circle so the handle is easy to grab
        if (QLineF(center, pos).length() <= circleRadius * 2.0) return pointIndex;
    }
    return -1;
}

int enzo::ui::Ramp::squareHitIndex_(const QPointF& pos) const
{
    const double squareCenterY = backgroundRect_().bottom();
    for (int pointIndex = 0; pointIndex < static_cast<int>(controlPoints_.size()); ++pointIndex)
    {
        const ControlPoint& controlPoint = controlPoints_[pointIndex];
        const double centerX = positionToX_(controlPoint.position);
        const QRectF squareRect(
            centerX - squareSize / 2.0 - 2.0,
            squareCenterY - squareSize / 2.0 - 2.0,
            squareSize + 4.0,
            squareSize + 4.0
        );
        if (squareRect.contains(pos)) return pointIndex;
    }
    return -1;
}

void enzo::ui::Ramp::mousePressEvent(QMouseEvent* event)
{
    const QPointF pos = event->position();

    // The host snapshots the pre edit state here before any point moves.
    Q_EMIT editBegan();

    // Circle takes priority so the value handle stays reachable when stacked on the square
    activePoint_ = circleHitIndex_(pos);
    if (activePoint_ != -1)
    {
        activeHandle_ = Handle::Circle;
        selectPoint_(activePoint_);
        return;
    }

    activePoint_ = squareHitIndex_(pos);
    if (activePoint_ != -1)
    {
        activeHandle_ = Handle::Square;
        selectPoint_(activePoint_);
        return;
    }

    // Empty space spawns a new control point grabbed by its circle for immediate dragging
    addControlPoint_(xToPosition_(pos.x()), yToValue_(pos.y()));
    activeHandle_ = Handle::Circle;
    Q_EMIT edited();
    selectPoint_(activePoint_);
    update();
}

void enzo::ui::Ramp::addControlPoint_(double position, double value)
{
    controlPoints_.push_back({0, position, value});
    sortAndRenumber_();
    activePoint_ = circleHitIndex_(QPointF(positionToX_(position), valueToY_(value)));
}

void enzo::ui::Ramp::selectPoint_(int pointIndex)
{
    if (pointIndex == selectedPoint_) return;
    selectedPoint_ = pointIndex;
    Q_EMIT selectionChanged(selectedPoint_);
}

void enzo::ui::Ramp::sortAndRenumber_()
{
    std::sort(
        controlPoints_.begin(),
        controlPoints_.end(),
        [](const ControlPoint& lhs, const ControlPoint& rhs) { return lhs.position < rhs.position; }
    );

    // Point numbers track linear order along the x axis
    for (int pointIndex = 0; pointIndex < static_cast<int>(controlPoints_.size()); ++pointIndex)
        controlPoints_[pointIndex].pointNumber = pointIndex;
}

void enzo::ui::Ramp::mouseMoveEvent(QMouseEvent* event)
{
    if (activePoint_ == -1) return;
    const QPointF pos = event->position();
    ControlPoint& controlPoint = controlPoints_[activePoint_];

    // Both handles share the control point x
    controlPoint.position = xToPosition_(pos.x());

    // Only the circle moves the value
    if (activeHandle_ == Handle::Circle) controlPoint.value = yToValue_(pos.y());

    // Reorder so the curve never folds back, then keep dragging the same point
    const double draggedPosition = controlPoint.position;
    const double draggedValue = controlPoint.value;
    sortAndRenumber_();
    for (int pointIndex = 0; pointIndex < static_cast<int>(controlPoints_.size()); ++pointIndex)
    {
        if (controlPoints_[pointIndex].position == draggedPosition &&
            controlPoints_[pointIndex].value == draggedValue)
        {
            activePoint_ = pointIndex;
            break;
        }
    }

    Q_EMIT edited();
    selectPoint_(activePoint_);
    update();
}

void enzo::ui::Ramp::mouseReleaseEvent(QMouseEvent*)
{
    activePoint_ = -1;
    activeHandle_ = Handle::None;
    Q_EMIT editEnded();
}
