#include "LegacyGui/UtilWidgets/BoolSwitch.h"
#include "LegacyGui/Style.h"
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <qnamespace.h>

enzo::ui::BoolSwitch::BoolSwitch(bool initialChecked, QWidget* parent) : QPushButton(parent)
{
    setFixedWidth(fullWidth_);
    setMinimumHeight(19);
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);

    setProperty("class", "BoolSwitch");
    setStyleSheet(QString(R"(
                  .BoolSwitch
                  {
                      border-radius: %1px;
                      border: 1px solid %2;
                  }
                  )")
                      .arg(enzo::style::parameter::borderRadius)
                      .arg(enzo::style::color::border.name()));

    setChecked(initialChecked);
    switchXEnd_ = fullWidth_ - handleWidth_ - 4;
    switchColor_ = initialChecked ? switchColorOn_ : switchColorOff_;
    switchX_ = initialChecked ? switchXEnd_ : 0;
}

void enzo::ui::BoolSwitch::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect bgRect = rect();
    painter.setPen(Qt::NoPen);
    painter.setBrush(switchColor_);
    // The handle radius follows the frame radius with an offset tuned by eye
    constexpr int borderRadius = enzo::style::parameter::borderRadius - 1;
    constexpr int margin = 2;
    painter.drawRoundedRect(
        QRectF(
            bgRect.left() + margin + switchX_,
            bgRect.top() + margin,
            handleWidth_,
            bgRect.height() - margin * 2
        ),
        borderRadius,
        borderRadius
    );
}

void enzo::ui::BoolSwitch::animateSwitch(bool checked)
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
