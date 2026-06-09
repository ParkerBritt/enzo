#include "Gui/UtilWidgets/Ramp.h"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <algorithm>

namespace {
// Inset from the widget edge to the background panel so handles can overflow it
constexpr double padding = enzo::ui::Ramp::panelInset;
constexpr double backgroundCornerRadius = 9.0;
const QColor panelColor("#1a1a1a");
const QColor borderColor("#383838");
const QColor circleColor("#B1B2B5");
const QColor selectedCircleColor("#ffffff");
const QColor squareColor("#808080");
const QColor curveStrokeColor("#B1B2B5");
const QColor curveFillTopColor(177, 178, 181, 100);
const QColor curveFillBottomColor(177, 178, 181, 10);
// Hitbox scale multiplier for circle handle
constexpr double circleHitRadiusScale = 4;
constexpr double dragScaleTarget = 0.9;
constexpr int hoverScaleDurationMs = 200;
} // namespace

enzo::ui::Ramp::Ramp(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(100);
    setMouseTracking(true);

    // Default endpoints rising from zero to one
    controlPoints_.push_back({0, 0.0, 0.0});
    controlPoints_.push_back({1, 1.0, 1.0});
}

void enzo::ui::Ramp::setPoints(
    const std::vector<QPointF>& points,
    const std::vector<prm::Interpolation>& interps
)
{
    controlPoints_.clear();
    controlPoints_.reserve(points.size());
    for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
        controlPoints_.push_back(
            {0, points[pointIndex].x(), points[pointIndex].y(), interps[pointIndex]}
        );
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

std::vector<enzo::prm::Interpolation> enzo::ui::Ramp::interps() const
{
    std::vector<prm::Interpolation> result;
    result.reserve(controlPoints_.size());
    for (const ControlPoint& controlPoint : controlPoints_)
        result.push_back(controlPoint.interp);
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
    painter.drawRoundedRect(backgroundRect_(), backgroundCornerRadius, backgroundCornerRadius);

    paintCurve_(painter);
    paintControlPoints_(painter);
}

std::vector<QPointF> enzo::ui::Ramp::buildCurveTop_() const
{
    std::vector<QPointF> topPoints;
    if (controlPoints_.empty()) return topPoints;

    const QRectF panel = backgroundRect_();

    // A snapshot of the points lets b spline segments sample with neighbour awareness.
    std::vector<prm::Ramp::Key> keys;
    keys.reserve(controlPoints_.size());
    for (const ControlPoint& controlPoint : controlPoints_)
        keys.push_back(
            {static_cast<floatT>(controlPoint.position),
             static_cast<floatT>(controlPoint.value),
             controlPoint.interp}
        );
    const prm::Ramp ramp(keys);

    // Flat extension from the left edge to the first control point
    topPoints.push_back(QPointF(panel.left(), valueToY_(controlPoints_.front().value)));
    topPoints.push_back(QPointF(
        positionToX_(controlPoints_.front().position),
        valueToY_(controlPoints_.front().value)
    ));

    for (std::size_t leftIndex = 0; leftIndex + 1 < controlPoints_.size(); ++leftIndex)
    {
        const ControlPoint& leftPoint = controlPoints_[leftIndex];
        const ControlPoint& rightPoint = controlPoints_[leftIndex + 1];
        const double leftX = positionToX_(leftPoint.position);
        const double rightX = positionToX_(rightPoint.position);

        // A run of b splines rounds toward the points bordering it, so a segment
        // curves when its right point or its left neighbour is a b spline.
        const bool previousCurves =
            leftIndex > 0 && controlPoints_[leftIndex - 1].interp == prm::Interpolation::BSPLINE;

        switch (leftPoint.interp)
        {
        case prm::Interpolation::CONSTANT:
            // Hold the left value across the segment then step up at the right point.
            topPoints.push_back(QPointF(rightX, valueToY_(leftPoint.value)));
            topPoints.push_back(QPointF(rightX, valueToY_(rightPoint.value)));
            break;
        case prm::Interpolation::BSPLINE:
            if (rightPoint.interp == prm::Interpolation::BSPLINE || previousCurves)
            {
                // Trace the curve at roughly one sample every few pixels.
                const int sampleCount = std::max(2, static_cast<int>((rightX - leftX) / 3.0));
                for (int sample = 1; sample <= sampleCount; ++sample)
                {
                    const double fraction = static_cast<double>(sample) / sampleCount;
                    const double position =
                        leftPoint.position + (rightPoint.position - leftPoint.position) * fraction;
                    topPoints.push_back(QPointF(
                        positionToX_(position),
                        valueToY_(ramp.sample(static_cast<floatT>(position)))
                    ));
                }
                break;
            }
            [[fallthrough]];
        case prm::Interpolation::LINEAR:
            topPoints.push_back(QPointF(rightX, valueToY_(rightPoint.value)));
            break;
        }
    }

    // Flat extension from the last control point to the right edge
    topPoints.push_back(QPointF(panel.right(), valueToY_(controlPoints_.back().value)));
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
    panelClip.addRoundedRect(panel, backgroundCornerRadius, backgroundCornerRadius);

    painter.save();
    painter.setClipPath(panelClip);

    QLinearGradient fillGradient(0, panel.top(), 0, panel.bottom());
    fillGradient.setColorAt(0.0, curveFillTopColor);
    fillGradient.setColorAt(1.0, curveFillBottomColor);

    painter.setPen(Qt::NoPen);
    painter.setBrush(fillGradient);
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
        bool isSelected = pointIndex == selectedPoint_;

        // Square centered on the background bottom edge sharing the control point x
        const QRectF squareRect(
            centerX - squareSize / 2.0,
            squareCenterY - squareSize / 2.0,
            squareSize,
            squareSize
        );
        painter.setPen(Qt::NoPen);
        painter.setBrush(squareColor);
        painter.drawRoundedRect(squareRect, 3, 3);

        // Free moving circle at the control point value, ringed when selected
        const double circleCenterY = valueToY_(controlPoint.value);
        const double radius =
            pointIndex == scaledPoint_ ? circleRadius * hoverScale_ : circleRadius;
        painter.setBrush(isSelected ? selectedCircleColor : circleColor);
        painter.setPen(QPen(Qt::NoPen));
        painter.drawEllipse(QPointF(centerX, circleCenterY), radius, radius);
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
        if (QLineF(center, pos).length() <= circleRadius * circleHitRadiusScale) return pointIndex;
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
    activeHandle_ = Handle::Circle;

    if (activePoint_ == -1)
    {
        activePoint_ = squareHitIndex_(pos);
        activeHandle_ = Handle::Square;
    }

    // Empty space spawns a new control point grabbed by its circle for immediate dragging
    if (activePoint_ == -1)
    {
        addControlPoint_(xToPosition_(pos.x()), yToValue_(pos.y()));
        activeHandle_ = Handle::Circle;
        Q_EMIT edited();
    }

    // The grabbed dot eases to its drag size while it is dragged
    scaledPoint_ = activePoint_;
    animateHoverScale_(dragScaleTarget, QEasingCurve::OutCubic);

    selectPoint_(activePoint_);
    update();
}

void enzo::ui::Ramp::addControlPoint_(double position, double value)
{
    controlPoints_.push_back({0, position, value});
    sortAndRenumber_();
    activePoint_ = circleHitIndex_(QPointF(positionToX_(position), valueToY_(value)));

    // A new point inherits the interp of the point on its left so the segment it
    // splits keeps the same shape it had before the split.
    if (activePoint_ > 0)
        controlPoints_[activePoint_].interp = controlPoints_[activePoint_ - 1].interp;
}

void enzo::ui::Ramp::selectPoint_(int pointIndex)
{
    if (pointIndex == selectedPoint_) return;
    selectedPoint_ = pointIndex;
    Q_EMIT selectionChanged(selectedPoint_);
}

void enzo::ui::Ramp::animateHoverScale_(qreal target, QEasingCurve::Type easing)
{
    if (qFuzzyCompare(scaleTarget_, target)) return;
    scaleTarget_ = target;

    auto grow = new QPropertyAnimation(this, "hoverScale", this);
    grow->setDuration(hoverScaleDurationMs);
    grow->setEasingCurve(easing);
    grow->setStartValue(hoverScale_);
    grow->setEndValue(target);
    grow->start(QAbstractAnimation::DeleteWhenStopped);
}

void enzo::ui::Ramp::hoverPoint_(int pointIndex)
{
    // A new dot starts from rest so it grows in cleanly, the old one snaps back
    if (pointIndex != -1 && pointIndex != scaledPoint_)
    {
        scaledPoint_ = pointIndex;
        setHoverScale(1.0);
    }

    const bool grow = pointIndex != -1;
    animateHoverScale_(
        grow ? hoverScaleTarget : 1.0,
        grow ? QEasingCurve::OutBack : QEasingCurve::OutCubic
    );
}

void enzo::ui::Ramp::leaveEvent(QEvent*) { hoverPoint_(-1); }

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
    const QPointF pos = event->position();

    // With no drag in progress the cursor just grows the dot it rests on
    if (activePoint_ == -1)
    {
        hoverPoint_(circleHitIndex_(pos));
        return;
    }

    ControlPoint& controlPoint = controlPoints_[activePoint_];

    // Both handles share the control point x
    controlPoint.position = xToPosition_(pos.x());

    // Only the circle moves the value, clamped to the visible panel
    if (activeHandle_ == Handle::Circle)
        controlPoint.value = std::clamp(yToValue_(pos.y()), valueMin_, valueMax_);

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

void enzo::ui::Ramp::mouseReleaseEvent(QMouseEvent* event)
{
    activePoint_ = -1;
    activeHandle_ = Handle::None;
    Q_EMIT editEnded();

    // Regrow the dot if the cursor settles back over one
    hoverPoint_(circleHitIndex_(event->position()));
}
