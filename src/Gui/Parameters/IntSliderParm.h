#pragma once
#include "Engine/Types.h"
#include "Gui/Parameters/SliderParmBase.h"
#include <QPen>

namespace enzo::ui {

class IntSliderParm : public SliderParmBase {
    Q_OBJECT
  public:
    IntSliderParm(std::weak_ptr<enzo::prm::Parameter> parameter, QWidget *parent = nullptr,
                  Qt::WindowFlags f = Qt::WindowFlags());

  protected:
    void paintEvent(QPaintEvent *event) override;
    void syncFromParameter() override;
    void applyValue(float normalizedValue) override;

  private:
    bt::intT value_;
    QPen notchPen_;
    static constexpr int notchWidth = 2;
    void setValueImpl(bt::intT value);
};

} // namespace enzo::ui
