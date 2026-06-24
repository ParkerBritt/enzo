#pragma once
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/UndoRedo/UndoDisabler.h"
#include "LegacyGui/Parameters/Parameter.h"
#include "LegacyGui/UtilWidgets/Slider.h"
#include <boost/signals2/connection.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace enzo::ui {

class XYZParm : public Parameter
{
    Q_OBJECT
  public:
    XYZParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent = nullptr);

  private:
    void onPressed(unsigned int vectorIndex);
    void onMoved(unsigned int vectorIndex, double value);
    void onReleased();
    void onExpressionEntered(unsigned int vectorIndex, const QString& expression);
    void syncFromParameter();

    std::weak_ptr<prm::NodeParameter> parameter_;
    std::vector<Slider*> sliders_;
    boost::signals2::scoped_connection valueChangedConnection_;
    std::optional<UndoDisabler> undoDisabler_;
    prm::PrmValues valueBeforeDrag_;
};

} // namespace enzo::ui
