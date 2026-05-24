#pragma once
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "Gui/Parameters/Parameter.h"
#include "Gui/UtilWidgets/Slider.h"
#include <boost/signals2/connection.hpp>
#include <memory>
#include <optional>

namespace enzo::ui {

class IntSliderParm : public Parameter {
    Q_OBJECT
  public:
    IntSliderParm(std::weak_ptr<enzo::prm::NodeParameter> parameter, QWidget *parent = nullptr);

  private:
    void onPressed();
    void onMoved(double value);
    void onReleased();
    void syncFromParameter();

    std::weak_ptr<prm::NodeParameter> parameter_;
    Slider* slider_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
    std::optional<UndoDisabler> undoDisabler_;
    prm::PrmValues valueBeforeDrag_;
};

} // namespace enzo::ui
