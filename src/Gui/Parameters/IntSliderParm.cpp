#include "Gui/Parameters/IntSliderParm.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <QPainter>
#include <QPaintEvent>
#include <QLabel>
#include <cmath>
#include <iostream>
#include <qboxlayout.h>
#include <qnamespace.h>
#include <algorithm>
#include <string>
#include <icecream.hpp>


enzo::ui::IntSliderParm::IntSliderParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    // tells qt to style the widget even though it's a Q_OBJECT
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(24);

    notchPen_ = QPen(QColor("#383838"), notchWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    parameter_=parameter;

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
        auto range = parameterShared->getTemplate().getRange();
        minValue_=range.getMin();
        maxValue_=range.getMax();
        clampMin_=range.getMinFlag()==prm::RangeFlag::LOCKED;
        clampMax_=range.getMaxFlag()==prm::RangeFlag::LOCKED;
        setValueImpl(parameterShared->evalInt());
    }
    else
    {
        throw std::bad_weak_ptr();
    }
}

void enzo::ui::IntSliderParm::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int valueRange = maxValue_-minValue_;
    float fillPercent = std::clamp<float>(static_cast<float>(value_-minValue_)/valueRange, 0, 1);
    float margin = 3;
    float fillWidth = rect().width()-margin*2;
    fillWidth *= fillPercent;

    QRectF fillRect = {rect().left()+margin, rect().top()+margin, fillWidth, rect().height()-margin*2};

    painter.setPen(notchPen_);
    QRectF markerLinesRect = rect();
    markerLinesRect.adjust(margin, margin, -margin, -margin);

    const int notchCount = std::min<int>(valueRange, 100);
    for(int i=1;i<notchCount; ++i)
    {
        float x = ((i-1)*markerLinesRect.width())/notchCount;
        x += notchWidth+4; // offset
        const float y = markerLinesRect.bottom()-2;
        painter.drawLine(x, y, x, y-5);

    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#383838"));
    painter.drawRoundedRect(fillRect, 6, 6);

}

void enzo::ui::IntSliderParm::setValueImpl(bt::intT value)
{
    if(clampMin_ && value<minValue_) { value = minValue_; }
    if(clampMax_ && value>maxValue_) { value = maxValue_; }

    value_ = value;
    QString valStr = QString::number(value_);
    valStr.truncate(4);
    valueLabel_->setText(valStr);


}

void enzo::ui::IntSliderParm::setValue(bt::intT value)
{

    setValueImpl(value);
    update();
    valueChanged(value_);

}


void enzo::ui::IntSliderParm::mouseMoveEvent(QMouseEvent *event)
{
    // normalized
    float value = static_cast<float>(event->pos().x())/rect().width();
    //remap
    value = minValue_+(maxValue_-minValue_)*value;
    setValue(rint(value));
}

void enzo::ui::IntSliderParm::mousePressEvent(QMouseEvent *event)
{
    // normalized
    float value = static_cast<float>(event->pos().x())/rect().width();
    //remap
    value = minValue_+(maxValue_-minValue_)*value;
    setValue(rint(value));
}

