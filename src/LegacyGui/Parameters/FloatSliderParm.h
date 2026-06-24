#pragma once
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "LegacyGui/Parameters/Parameter.h"
#include "LegacyGui/UtilWidgets/Slider.h"
#include <boost/signals2/connection.hpp>
#include <memory>
#include <optional>

namespace enzo::ui {

class FloatSliderParm : public Parameter
{
    Q_OBJECT
  public:
    FloatSliderParm(
        std::weak_ptr<prm::NodeParameter> parameter,
        unsigned int vectorIndex = 0,
        QWidget* parent = nullptr
    );

  private:
    void onPressed();
    void onMoved(double value);
    void onReleased();
    void onExpressionEntered(const QString& expression);
    void syncFromParameter();

    std::weak_ptr<prm::NodeParameter> parameter_;
    unsigned int vectorIndex_;
    Slider* slider_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
    std::optional<UndoDisabler> undoDisabler_;
    prm::PrmValues valueBeforeDrag_;
};

} // namespace enzo::ui
