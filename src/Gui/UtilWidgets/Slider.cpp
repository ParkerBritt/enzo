#include "Gui/UtilWidgets/Slider.h"
#include "Gui/Style.h"
#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
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
    setFixedHeight(enzo::style::parameter::height);
    setProperty("type", "Slider");
    setStyleSheet(QString(R"(
                  QWidget[type="Slider"]
                  {
                      border-radius: %1px;
                      border: 1px solid %2;
                  }
                  )")
                      .arg(enzo::style::parameter::borderRadius)
                      .arg(enzo::style::color::border.name()));
    notchPen_ =
        QPen(enzo::style::slider::trackColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    value_ = clampAndStep_(minValue_);
}

void enzo::ui::Slider::setValue(double value)
{
    const double clamped = clampAndStep_(value);
    if (clamped == value_) return;
    value_ = clamped;
    update();
}

void enzo::ui::Slider::setRange(double minValue, double maxValue)
{
    minValue_ = minValue;
    maxValue_ = maxValue;
    value_ = clampAndStep_(value_);
    update();
}

void enzo::ui::Slider::setDisplayPrecision(int digits)
{
    displayDigits_ = digits;
    update();
}

void enzo::ui::Slider::setExpressionText(const QString& text)
{
    if (expressionText_ == text) return;
    expressionText_ = text;
    update();
}

void enzo::ui::Slider::setExpressionHasError(bool hasError)
{
    if (expressionHasError_ == hasError) return;
    expressionHasError_ = hasError;
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
    // The fill radius follows the frame radius with an offset tuned by eye
    constexpr double cornerRadius = enzo::style::parameter::borderRadius - 1;
    QPainterPath fullBar;
    fullBar.addRoundedRect(trackRect, cornerRadius, cornerRadius);
    QPainterPath fillMask;
    fillMask.addRoundedRect(fillRect, cornerRadius, cornerRadius);

    painter.save();
    painter.setClipPath(fillMask);
    painter.fillPath(fullBar, enzo::style::slider::trackColor);
    painter.restore();
}

void enzo::ui::Slider::paintValueText_(QPainter& painter) const
{
    // The editor draws its own text while open
    if (editor_ && editor_->isVisible()) return;

    // An expression shows in a distinct color in place of the numeric value,
    // blue when it evaluates and red when it fails.
    if (!expressionText_.isEmpty())
    {
        painter.setPen(
            expressionHasError_ ? enzo::style::slider::expressionColorError
                                : enzo::style::slider::expressionColor
        );
        painter.drawText(rect(), Qt::AlignCenter, expressionText_);
        return;
    }

    QString valueText = QString::number(value_, 'f', displayDigits_);
    painter.setPen(enzo::style::slider::foregroundColor);
    painter.drawText(rect(), Qt::AlignCenter, valueText);
}

void enzo::ui::Slider::mousePressEvent(QMouseEvent* event)
{
    // Defer commitment until a release decides between a click and a drag
    pressPos_ = event->pos();
    dragging_ = false;
}

void enzo::ui::Slider::mouseMoveEvent(QMouseEvent* event)
{
    if (!dragging_)
    {
        if ((event->pos() - pressPos_).manhattanLength() < QApplication::startDragDistance())
            return;
        dragging_ = true;
        Q_EMIT sliderPressed();
    }
    const double normalized = static_cast<double>(event->pos().x()) / rect().width();
    emitMoved_(normalized);
}

void enzo::ui::Slider::mouseReleaseEvent(QMouseEvent*)
{
    if (dragging_)
    {
        dragging_ = false;
        Q_EMIT sliderReleased();
    }
    else
    {
        beginEditing_();
    }
}

void enzo::ui::Slider::beginEditing_()
{
    if (!editor_)
    {
        // Transparent so the slider track and fill stay visible underneath
        editor_ = new QLineEdit(this);
        editor_->setAlignment(Qt::AlignCenter);
        editor_->setFrame(false);
        editor_->setStyleSheet(QString(R"(
                      QLineEdit
                      {
                          background: transparent;
                          border: none;
                          color: %1;
                      }
                      )")
                                   .arg(enzo::style::color::textMuted.name()));
        connect(editor_, &QLineEdit::editingFinished, this, &Slider::commitEditing_);
    }
    editor_->setGeometry(rect());
    // Editing an expression starts from its source, a literal from its number.
    editor_->setText(
        expressionText_.isEmpty() ? QString::number(value_, 'f', displayDigits_) : expressionText_
    );
    editor_->selectAll();
    editor_->show();
    editor_->setFocus();

    // Watch the whole app so a click on any widget outside the editor exits
    qApp->installEventFilter(this);
}

void enzo::ui::Slider::commitEditing_()
{
    if (!editor_ || editor_->isHidden()) return;

    const QString typedText = editor_->text().trimmed();
    endEditing_();

    // A pure number commits as a value, anything else commits as an expression.
    bool isNumber = false;
    const double typedValue = typedText.toDouble(&isNumber);
    if (!isNumber)
    {
        Q_EMIT expressionEntered(typedText);
        return;
    }

    const double newValue = clampAndStep_(typedValue);
    Q_EMIT sliderPressed();
    value_ = newValue;
    update();
    Q_EMIT sliderMoved(newValue);
    Q_EMIT sliderReleased();
}

void enzo::ui::Slider::endEditing_()
{
    editor_->hide();
    qApp->removeEventFilter(this);
}

bool enzo::ui::Slider::eventFilter(QObject* watched, QEvent* event)
{
    if (editor_ && editor_->isVisible())
    {
        // Escape leaves edit mode without changing the value
        if (event->type() == QEvent::KeyPress &&
            static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
        {
            endEditing_();
            return true;
        }

        // A click on any widget outside the editor commits and exits
        if (event->type() == QEvent::MouseButtonPress && watched != editor_)
        {
            commitEditing_();
        }
    }
    return QWidget::eventFilter(watched, event);
}
