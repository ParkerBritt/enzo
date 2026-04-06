#include "Gui/Parameters/FloatSliderParm.h"
#include <QPaintEvent>
#include <QPainter>
#include <algorithm>

enzo::ui::FloatSliderParm::FloatSliderParm(std::weak_ptr<prm::Parameter> parameter,
                                           unsigned int vectorIndex, QWidget *parent,
                                           Qt::WindowFlags f)
    : SliderParmBase(parameter, vectorIndex, parent, f) {
    setStyleSheet(R"(
                  QWidget[type="SliderParm"]
                  {
                      border-radius: 6px;
                      border: 1px solid #303030;
                  }
                  )");
    syncFromParameter();
}

void enzo::ui::FloatSliderParm::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#383838"));

    const int valueRange = maxValue_ - minValue_;
    float fillPercent =
        std::clamp<float>(static_cast<float>(value_ - minValue_) / valueRange, 0, 1);

    constexpr float margin = 3;
    float fillWidth = rect().width() - margin * 2;
    fillWidth *= fillPercent;

    QRectF fillRect = {rect().left() + margin, rect().top() + margin, fillWidth,
                       rect().height() - margin * 2};
    painter.drawRoundedRect(fillRect, 6, 6);
}

void enzo::ui::FloatSliderParm::syncFromParameter() {
    if (auto parameterShared = parameter_.lock()) {
        setValueImpl(parameterShared->evalFloat(vectorIndex_));
    }
}

void enzo::ui::FloatSliderParm::applyValue(float normalizedValue) {
    float value = minValue_ + (maxValue_ - minValue_) * normalizedValue;
    if (auto parameterShared = parameter_.lock()) {
        parameterShared->setFloat(value, vectorIndex_);
    }
}

void enzo::ui::FloatSliderParm::setValueImpl(bt::floatT value) {
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
