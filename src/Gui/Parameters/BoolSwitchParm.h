#pragma once

#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include <memory>
#include <QWidget>
#include <qpushbutton.h>
#include <QPropertyAnimation>
#include <icecream.hpp>

namespace enzo::ui
{
class BoolSwitchParm
: public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal switchX READ switchX WRITE setSwitchX)
    Q_PROPERTY(QColor switchColor READ switchColor WRITE setSwitchColor)
public:
    BoolSwitchParm(std::weak_ptr<enzo::prm::Parameter> parameter, QWidget *parent = nullptr);
private:
    std::weak_ptr<enzo::prm::Parameter> parameter_;
    qreal switchX_=0;
    qreal switchXEnd_;
    QColor switchColorOff_=QColor("#383838");
    QColor switchColorOn_=QColor("#B3B3B3");
    QColor switchColor_;

    void setValue(bt::intT);
    void onToggle(bool checked);

    qreal switchX() const { return switchX_; }
    void setSwitchX(qreal x) { switchX_ = x; update(); }
    QColor switchColor() const { return switchColor_; }
    void setSwitchColor(const QColor &c) { switchColor_ = c; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private Q_SLOTS:
    void animateSwitch(bool checked);
Q_SIGNALS:
    void valueChanged(bt::intT value); 
};
}
