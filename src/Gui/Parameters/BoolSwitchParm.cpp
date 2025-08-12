#include "Gui/Parameters/BoolSwitchParm.h"
#include <qnamespace.h>
#include <qpushbutton.h>
#include <QPainter>
#include <QParallelAnimationGroup>

enzo::ui::BoolSwitchParm::BoolSwitchParm(std::weak_ptr<enzo::prm::Parameter> parameter, QWidget *parent)
: QPushButton(parent), parameter_{parameter}
{
    setFixedWidth(40);
    parameter_ = parameter;
    setCheckable(true);

    setProperty("class", "BoolSwitchParm");
    setStyleSheet(R"(
                  .BoolSwitchParm
                  {
                      border-radius: 8px;
                      border: 1px solid #383838;
                  }
                  )");

    if(auto parameterShared = parameter_.lock())
    {
        bool toggled = parameterShared->evalInt();
        setChecked(toggled);
        switchXEnd_=width() - 20 - 4;
        switchColor_= toggled ? switchColorOn_ : switchColorOff_;
        switchX_= toggled ? switchXEnd_ : 0;
    }
    connect(this, &QPushButton::toggled, this, &BoolSwitchParm::onToggle);

}

void enzo::ui::BoolSwitchParm::onToggle(bool checked)
{
    animateSwitch(checked);
    setValue(checked);
}


void enzo::ui::BoolSwitchParm::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event); // paint bg
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect bgRect = rect();

    painter.setPen(Qt::NoPen);
    painter.setBrush(switchColor_);
    constexpr int borderRadius = 7;
    constexpr int margin = 2;
    painter.drawRoundedRect(QRectF(bgRect.left()+margin+switchX_, bgRect.top()+margin, 20, bgRect.height()-margin*2), borderRadius, borderRadius);
}

void enzo::ui::BoolSwitchParm::setValue(bt::intT value)
{
    setChecked(value);
    valueChanged(value);
}

void enzo::ui::BoolSwitchParm::animateSwitch(bool checked)
{
    auto posAnim = new QPropertyAnimation(this, "switchX");
    posAnim->setDuration(200);
    posAnim->setEasingCurve(QEasingCurve::InOutQuad);
    posAnim->setStartValue(switchX_);
    posAnim->setEndValue(checked ? switchXEnd_ : 0);

    auto colorAnim = new QPropertyAnimation(this, "switchColor");
    colorAnim->setDuration(200);
    colorAnim->setEasingCurve(QEasingCurve::InOutQuad);
    colorAnim->setStartValue(switchColor_);
    colorAnim->setEndValue(checked ? switchColorOn_ : switchColorOff_);

    auto group = new QParallelAnimationGroup(this);
    group->addAnimation(posAnim);
    group->addAnimation(colorAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

