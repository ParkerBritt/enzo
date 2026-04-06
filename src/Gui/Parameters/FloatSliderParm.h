#pragma once
#include "Engine/Types.h"
#include "Gui/Parameters/SliderParmBase.h"

namespace enzo::ui {

class FloatSliderParm : public SliderParmBase {
    Q_OBJECT
  public:
    FloatSliderParm(std::weak_ptr<prm::Parameter> parameter, unsigned int vectorIndex = 0,
                    QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  protected:
    void paintEvent(QPaintEvent *event) override;
    void syncFromParameter() override;
    void applyValue(float normalizedValue) override;

  private:
    bt::floatT value_;
    void setValueImpl(bt::floatT value);
};

} // namespace enzo::ui
