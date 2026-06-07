#include "Gui/UtilWidgets/Ramp.h"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <algorithm>

namespace {
constexpr double margin = 12.0;
constexpr double circleRadius = 5.0;
constexpr double squareSize = 9.0;
const QColor circleColor("#ffffff");
const QColor squareColor("#B1B2B5");
} // namespace

enzo::ui::Ramp::Ramp(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(100);
    setProperty("type", "Ramp");
    setStyleSheet(R"(
                  QWidget[type="Ramp"]
                  {
                      border-radius: 9px;
                      border: 1px solid #383838;
                  }
                  )");

    // Default endpoints rising from zero to one
    controlPoints_.push_back({0, 0.0, 0.0});
    controlPoints_.push_back({1, 1.0, 1.0});
}

double enzo::ui::Ramp::positionToX_(double position) const
{
    const double left = margin;
    const double right = width() - margin;
    return left + (right - left) * position;
}

double enzo::ui::Ramp::valueToY_(double value) const
{
    const double bottom = height() - margin;
    const double top = margin;
    const double normalized = (value - valueMin_) / (valueMax_ - valueMin_);
    return bottom - (bottom - top) * normalized;
}

double enzo::ui::Ramp::xToPosition_(double x) const
{
    const double left = margin;
    const double right = width() - margin;
    return std::clamp((x - left) / (right - left), 0.0, 1.0);
}

double enzo::ui::Ramp::yToValue_(double y) const
{
    const double bottom = height() - margin;
    const double top = margin;
    const double normalized = (bottom - y) / (bottom - top);
    return valueMin_ + (valueMax_ - valueMin_) * normalized;
}

void enzo::ui::Ramp::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    paintControlPoints_(painter);
}

void enzo::ui::Ramp::paintControlPoints_(QPainter& painter) const
{
    const double squareCenterY = height() - margin - squareSize / 2.0;

    for (const ControlPoint& controlPoint : controlPoints_)
    {
        const double centerX = positionToX_(controlPoint.position);

        // Bottom anchored square sharing the control point x
        const QRectF squareRect(
            centerX - squareSize / 2.0,
            squareCenterY - squareSize / 2.0,
            squareSize,
            squareSize
        );
        painter.setPen(Qt::NoPen);
        painter.setBrush(squareColor);
        painter.drawRect(squareRect);

        // Free moving circle at the control point value
        const double circleCenterY = valueToY_(controlPoint.value);
        painter.setBrush(circleColor);
        painter.drawEllipse(QPointF(centerX, circleCenterY), circleRadius, circleRadius);
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
    const double squareCenterY = height() - margin - squareSize / 2.0;
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

    // Circle takes priority so the value handle stays reachable when stacked on the square
    activePoint_ = circleHitIndex_(pos);
    if (activePoint_ != -1)
    {
        activeHandle_ = Handle::Circle;
        return;
    }

    activePoint_ = squareHitIndex_(pos);
    if (activePoint_ != -1)
    {
        activeHandle_ = Handle::Square;
        return;
    }

    // Empty space spawns a new control point grabbed by its circle for immediate dragging
    addControlPoint_(xToPosition_(pos.x()), yToValue_(pos.y()));
    activeHandle_ = Handle::Circle;
    update();
}

void enzo::ui::Ramp::addControlPoint_(double position, double value)
{
    controlPoints_.push_back({0, position, value});
    std::sort(
        controlPoints_.begin(),
        controlPoints_.end(),
        [](const ControlPoint& lhs, const ControlPoint& rhs) { return lhs.position < rhs.position; }
    );

    // Point numbers track linear order along the x axis
    for (int pointIndex = 0; pointIndex < static_cast<int>(controlPoints_.size()); ++pointIndex)
        controlPoints_[pointIndex].pointNumber = pointIndex;

    activePoint_ = circleHitIndex_(QPointF(positionToX_(position), valueToY_(value)));
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

    update();
}

void enzo::ui::Ramp::mouseReleaseEvent(QMouseEvent*)
{
    activePoint_ = -1;
    activeHandle_ = Handle::None;
}
