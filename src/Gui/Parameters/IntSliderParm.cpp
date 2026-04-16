#include "Gui/Parameters/IntSliderParm.h"
#include <QPaintEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

enzo::ui::IntSliderParm::IntSliderParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent,
                                       Qt::WindowFlags f)
    : SliderParmBase(parameter, 0, parent, f) {
    notchPen_ = QPen(QColor("#383838"), notchWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    setStyleSheet(R"(
                  QWidget[type="SliderParm"]
                  {
                      border-radius: 6px;
                      border: 1px solid #383838;
                  }
                  )");
    syncFromParameter();
}

void enzo::ui::IntSliderParm::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int valueRange = maxValue_ - minValue_;
    float fillPercent =
        std::clamp<float>(static_cast<float>(value_ - minValue_) / valueRange, 0, 1);
    float margin = 3;
    float fillWidth = rect().width() - margin * 2;
    fillWidth *= fillPercent;

    QRectF fillRect = {rect().left() + margin, rect().top() + margin, fillWidth,
                       rect().height() - margin * 2};

    painter.setPen(notchPen_);
    QRectF markerLinesRect = rect();
    markerLinesRect.adjust(margin, margin, -margin, -margin);

    const int notchCount = std::min<int>(valueRange, 100);
    for (int i = 1; i < notchCount; ++i) {
        float x = ((i - 1) * markerLinesRect.width()) / notchCount;
        x += notchWidth + 4; // offset
        const float y = markerLinesRect.bottom() - 2;
        painter.drawLine(x, y, x, y - 5);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#383838"));
    painter.drawRoundedRect(fillRect, 6, 6);
}

void enzo::ui::IntSliderParm::syncFromParameter() {
    if (auto parameterShared = parameter_.lock()) {
        setValueImpl(parameterShared->evalInt(vectorIndex_));
    }
}

void enzo::ui::IntSliderParm::applyValue(float normalizedValue) {
    float value = minValue_ + (maxValue_ - minValue_) * normalizedValue;
    if (auto parameterShared = parameter_.lock()) {
        parameterShared->setInt(rint(value), vectorIndex_);
    }
}

void enzo::ui::IntSliderParm::setValueImpl(bt::intT value) {
    if (clampMin_ && value < minValue_) {
        value = minValue_;
    }
    if (clampMax_ && value > maxValue_) {
        value = maxValue_;
    }

    value_ = value;
    QString valStr = QString::number(value_);
    valStr.truncate(4);
    valueLabel_->setText(valStr);
}
