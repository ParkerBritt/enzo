#pragma once

#include <QColor>
#include <QPushButton>
#include <qtmetamacros.h>

namespace enzo::ui {

class BoolSwitch : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal switchX READ switchX WRITE setSwitchX)
    Q_PROPERTY(QColor switchColor READ switchColor WRITE setSwitchColor)
  public:
    BoolSwitch(bool initialChecked, QWidget* parent = nullptr);
    void animateSwitch(bool checked);

  protected:
    void paintEvent(QPaintEvent*) override;

  private:
    int fullWidth_ = 35;
    int handleWidth_ = 17;
    qreal switchX_ = 0;
    qreal switchXEnd_ = 0;
    QColor switchColorOff_ = QColor("#383838");
    QColor switchColorOn_ = QColor("#B3B3B3");
    QColor switchColor_;

    qreal switchX() const { return switchX_; }
    void setSwitchX(qreal x)
    {
        switchX_ = x;
        update();
    }
    QColor switchColor() const { return switchColor_; }
    void setSwitchColor(const QColor& c)
    {
        switchColor_ = c;
        update();
    }
};

} // namespace enzo::ui
