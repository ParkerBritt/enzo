#include "Gui/UtilWidgets/Slider.h"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <algorithm>
#include <cmath>

enzo::ui::Slider::Slider(double minValue, double maxValue, bool clampMin, bool clampMax,
                         double step, QWidget* parent)
: QWidget(parent),
  minValue_(minValue),
  maxValue_(maxValue),
  clampMin_(clampMin),
  clampMax_(clampMax),
  step_(step)
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
    const float fillPercent = valueRange > 0
        ? static_cast<float>(std::clamp<double>((value_ - minValue_) / valueRange, 0.0, 1.0))
        : 0.0f;
    float fillWidth = (rect().width() - margin * 2) * fillPercent;
    QRectF fillRect = {rect().left() + margin, rect().top() + margin, fillWidth,
                       rect().height() - margin * 2};

    // Discrete step notches drawn when steps fit reasonably in the bar
    if (step_ > 0.0 && valueRange > 0.0)
    {
        const int notchCount = std::min<int>(static_cast<int>(valueRange / step_), 100);
        if (notchCount > 1)
        {
            painter.setPen(notchPen_);
            QRectF notchRect = rect();
            notchRect.adjust(margin, margin, -margin, -margin);
            for (int i = 1; i < notchCount; ++i)
            {
                float x = ((i - 1) * notchRect.width()) / notchCount + 1 + 4;
                const float y = notchRect.bottom() - 2;
                painter.drawLine(x, y, x, y - 5);
            }
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#383838"));
    painter.drawRoundedRect(fillRect, 8, 8);

    QString valStr = QString::number(value_, 'g', displayDigits_);
    valStr.truncate(displayDigits_);
    painter.setPen(QColor("#B3B3B3"));
    painter.drawText(rect(), Qt::AlignCenter, valStr);
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

void enzo::ui::Slider::mouseReleaseEvent(QMouseEvent*)
{
    Q_EMIT sliderReleased();
}
