#pragma once

#include <QPen>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <qtmetamacros.h>

class QPainter;
class QRectF;
class QLineEdit;

namespace enzo::ui {

/**
 * @brief Reusable slider widget that draws a horizontal fill bar and emits
 * value changes from mouse drag. Hosts pass a step of 0 for continuous float
 * behavior, or a positive step for discrete int behavior.
 */
class Slider : public QWidget
{
    Q_OBJECT
  public:
    Slider(
        double minValue,
        double maxValue,
        bool clampMin,
        bool clampMax,
        double step = 0.0,
        QWidget* parent = nullptr
    );

    double value() const { return value_; }
    void setValue(double value);
    void setRange(double minValue, double maxValue);
    void setDisplayPrecision(int digits);

    /// @brief Shows an expression in place of the numeric value, empty to hide it.
    void setExpressionText(const QString& text);

    /// @brief Colors the shown expression red when it failed to evaluate.
    void setExpressionHasError(bool hasError);

  Q_SIGNALS:
    void sliderPressed();
    void sliderMoved(double value);
    void sliderReleased();

    // Emitted when the typed text is not a plain number, so the host can treat it
    // as an expression.
    void expressionEntered(const QString& expression);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    double normalizedToValue_(double normalized) const;
    double clampAndStep_(double value) const;
    void emitMoved_(double normalized);

    void beginEditing_();
    void commitEditing_();
    void endEditing_();

    void paintNotches_(QPainter& painter, const QRectF& trackRect) const;
    void paintFill_(QPainter& painter, const QRectF& trackRect, const QRectF& fillRect) const;
    void paintValueText_(QPainter& painter) const;

    double minValue_;
    double maxValue_;
    bool clampMin_;
    bool clampMax_;
    double step_;
    double value_ = 0.0;
    int displayDigits_ = 2;

    // The expression shown in place of the value, empty when the value is a literal.
    QString expressionText_;
    // True when the shown expression failed to evaluate, drawing it red.
    bool expressionHasError_ = false;

    QPen notchPen_;

    // Click opens text entry, drag past a small threshold slides instead
    QLineEdit* editor_ = nullptr;
    QPoint pressPos_;
    bool dragging_ = false;
};

} // namespace enzo::ui
