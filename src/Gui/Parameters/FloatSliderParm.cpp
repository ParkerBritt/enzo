#include "Gui/Parameters/FloatSliderParm.h"
#include "Engine/Types.h"
#include <QPainter>
#include <QPaintEvent>
#include <QLabel>
#include <iostream>
#include <qboxlayout.h>
#include <qnamespace.h>
#include <icecream.hpp>
#include <algorithm>
#include <string>


enzo::ui::FloatSliderParm::FloatSliderParm(std::weak_ptr<prm::Parameter> parameter, unsigned int vectorIndex, QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    // tells qt to style the widget even though it's a Q_OBJECT
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(24);

    parameter_ = parameter;
    
    mainLayout_ = new QVBoxLayout();
    setLayout(mainLayout_);

    valueLabel_ = new QLabel();
    valueLabel_->setAlignment(Qt::AlignCenter);
    valueLabel_->setStyleSheet("background-color: none;");
    setProperty("type", "SliderParm");
    setStyleSheet(R"(
                  QWidget[type="SliderParm"]
                  {
                      border-radius: 6px;
                      border: 1px solid #383838;
                  }
                  )");
    mainLayout_->addWidget(valueLabel_);

    if(auto parameterShared=parameter_.lock())
    {
        auto range = parameterShared->getTemplate().getRange(vectorIndex);
        minValue_=range.getMin();
        maxValue_=range.getMax();
        clampMin_=range.getMinFlag()==prm::RangeFlag::LOCKED;
        clampMax_=range.getMaxFlag()==prm::RangeFlag::LOCKED;
        setValueImpl(parameterShared->evalFloat(vectorIndex));
    }
    else
    {
        throw std::bad_weak_ptr();
    }
}

void enzo::ui::FloatSliderParm::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#383838"));

    const int valueRange = maxValue_-minValue_;
    float fillPercent = std::clamp<float>(static_cast<float>(value_-minValue_)/valueRange, 0, 1);

    constexpr float margin = 3;
    float fillWidth = rect().width()-margin*2;
    fillWidth *= fillPercent;

    QRectF fillRect = {rect().left()+margin, rect().top()+margin, fillWidth, rect().height()-margin*2};
    painter.drawRoundedRect(fillRect, 6, 6);

}

void enzo::ui::FloatSliderParm::setValueImpl(bt::floatT value)
{
    if(clampMin_ && value<minValue_) { value = minValue_; }
    if(clampMax_ && value>maxValue_) { value = maxValue_; }

    value_ = value;
    QString valStr = QString::number(value_);
    valStr.truncate(4);
    valueLabel_->setText(valStr);


}

void enzo::ui::FloatSliderParm::setValue(bt::floatT value)
{

    setValueImpl(value);
    update();
    valueChanged(value_);

}


void enzo::ui::FloatSliderParm::mouseMoveEvent(QMouseEvent *event)
{
    // normalized
    float value = static_cast<float>(event->pos().x())/rect().width();
    //remap
    value = minValue_+(maxValue_-minValue_)*value;
    setValue(value);
}

void enzo::ui::FloatSliderParm::mousePressEvent(QMouseEvent *event)
{
    // normalized
    float value = static_cast<float>(event->pos().x())/rect().width();
    //remap
    value = minValue_+(maxValue_-minValue_)*value;
    setValue(value);
}

