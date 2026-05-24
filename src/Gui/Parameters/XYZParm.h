#pragma once
#include "Engine/Parameter/Parameter.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "Gui/Parameters/Parameter.h"
#include "Gui/UtilWidgets/Slider.h"
#include <boost/signals2/connection.hpp>
#include <vector>
#include <memory>
#include <optional>

namespace enzo::ui {

class XYZParm : public Parameter {
    Q_OBJECT
  public:
    XYZParm(std::weak_ptr<prm::Parameter> parameter, QWidget *parent = nullptr);

  private:
    void onPressed(unsigned int vectorIndex);
    void onMoved(unsigned int vectorIndex, double value);
    void onReleased();
    void syncFromParameter();

    std::weak_ptr<prm::Parameter> parameter_;
    std::vector<Slider*> sliders_;
    boost::signals2::scoped_connection valueChangedConnection_;
    std::optional<UndoDisabler> undoDisabler_;
    prm::PrmValues valueBeforeDrag_;
};

} // namespace enzo::ui
