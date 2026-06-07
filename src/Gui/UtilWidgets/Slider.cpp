#include "Gui/UtilWidgets/Slider.h"
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

enzo::ui::Slider::Slider(
    double minValue,
    double maxValue,
    bool clampMin,
    bool clampMax,
    double step,
    QWidget* parent
)
    : QWidget(parent), minValue_(minValue), maxValue_(maxValue), clampMin_(clampMin),
      clampMax_(clampMax), step_(step)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(24);
    setProperty("type", "Slider");
    setStyleSheet(R"(
                  QWidget[type="Slider"]
                  {
                      border-radius: 9px;
                      border: 1px solid #383838;
                  }
                  )");
    notchPen_ = QPen(QColor("#383838"), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    value_ = clampAndStep_(minValue_);
}

void enzo::ui::Slider::setValue(double value)
{
    const double clamped = clampAndStep_(value);
    if (clamped == value_) return;
    value_ = clamped;
    update();
}

void enzo::ui::Slider::setDisplayPrecision(int digits)
{
    displayDigits_ = digits;
    update();
}

double enzo::ui::Slider::clampAndStep_(double value) const
{
    if (clampMin_ && value < minValue_) value = minValue_;
    if (clampMax_ && value > maxValue_) value = maxValue_;
    if (step_ > 0.0) value = std::round(value / step_) * step_;
    return value;
}

double enzo::ui::Slider::normalizedToValue_(double normalized) const
{
    return minValue_ + (maxValue_ - minValue_) * normalized;
}

void enzo::ui::Slider::emitMoved_(double normalized)
{
    const double newValue = clampAndStep_(normalizedToValue_(normalized));
    value_ = newValue;
    update();
    Q_EMIT sliderMoved(newValue);
}

void enzo::ui::Slider::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    constexpr float margin = 3;
    const double valueRange = maxValue_ - minValue_;
    const float fillPercent =
        valueRange > 0
            ? static_cast<float>(std::clamp<double>((value_ - minValue_) / valueRange, 0.0, 1.0))
            : 0.0f;

    const QRectF trackRect = QRectF(rect()).adjusted(margin, margin, -margin, -margin);
    QRectF fillRect = trackRect;
    fillRect.setWidth(trackRect.width() * fillPercent);

    paintNotches_(painter, trackRect);
    paintFill_(painter, trackRect, fillRect);
    paintValueText_(painter);
}

void enzo::ui::Slider::paintNotches_(QPainter& painter, const QRectF& trackRect) const
{
    // Discrete step notches drawn when steps fit reasonably in the bar
    const double valueRange = maxValue_ - minValue_;
    if (step_ <= 0.0 || valueRange <= 0.0) return;

    const int notchCount = std::min<int>(static_cast<int>(valueRange / step_), 100);
    if (notchCount <= 1) return;

    painter.setPen(notchPen_);
    for (int notchIndex = 1; notchIndex < notchCount; ++notchIndex)
    {
        const float x = ((notchIndex - 1) * trackRect.width()) / notchCount + 5;
        const float y = trackRect.bottom() - 2;
        painter.drawLine(x, y, x, y - 5);
    }
}

void enzo::ui::Slider::paintFill_(
    QPainter& painter,
    const QRectF& trackRect,
    const QRectF& fillRect
) const
{
    // Reveal the full length rounded bar through a rounded mask at the fill width
    // so the anchored left end keeps its radius and the moving right end stays round
    constexpr double cornerRadius = 8;
    QPainterPath fullBar;
    fullBar.addRoundedRect(trackRect, cornerRadius, cornerRadius);
    QPainterPath fillMask;
    fillMask.addRoundedRect(fillRect, cornerRadius, cornerRadius);

    painter.save();
    painter.setClipPath(fillMask);
    painter.fillPath(fullBar, QColor("#383838"));
    painter.restore();
}

void enzo::ui::Slider::paintValueText_(QPainter& painter) const
{
    QString valueText = QString::number(value_, 'g', displayDigits_);
    valueText.truncate(displayDigits_);
    painter.setPen(QColor("#B3B3B3"));
    painter.drawText(rect(), Qt::AlignCenter, valueText);
}

void enzo::ui::Slider::mousePressEvent(QMouseEvent* event)
{
    Q_EMIT sliderPressed();
    const double normalized = static_cast<double>(event->pos().x()) / rect().width();
    emitMoved_(normalized);
}

void enzo::ui::Slider::mouseMoveEvent(QMouseEvent* event)
{
    const double normalized = static_cast<double>(event->pos().x()) / rect().width();
    emitMoved_(normalized);
}

void enzo::ui::Slider::mouseReleaseEvent(QMouseEvent*) { Q_EMIT sliderReleased(); }
