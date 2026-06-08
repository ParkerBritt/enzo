#pragma once
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include "Gui/Parameters/Parameter.h"
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui {

class Ramp;
class Slider;
class Dropdown;

/**
 * @brief Editor for a ramp multiparm. A curve widget edits every control point
 * directly while sliders below pick an instance and tune its position and value.
 */
class RampParm : public Parameter
{
    Q_OBJECT
  public:
    RampParm(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent = nullptr);

  private:
    void onRampEdited();
    void onIndexMoved(double value);
    void onPositionMoved(double value);
    void onValueMoved(double value);
    void onInterpChanged();

    void beginEdit();
    void commitEdit();

    void selectInstance(int instanceIndex);
    void syncFromParameter();
    void syncSelectedFields();

    std::shared_ptr<prm::Parameter> selectedField_(std::string_view fieldName) const;

    std::weak_ptr<prm::NodeParameter> parameter_;
    Ramp* rampWidget_ = nullptr;
    Slider* indexSlider_ = nullptr;
    Slider* positionSlider_ = nullptr;
    Slider* valueSlider_ = nullptr;
    Dropdown* interpDropdown_ = nullptr;

    int selectedInstance_ = 0;
    // Set while writing ramp edits into the parameter so the echoed valueChanged
    // does not rebuild the curve out from under an active drag.
    bool applyingFromRamp_ = false;
    // Whole multiparm state captured when an edit begins so its release can push
    // one undo step.
    ParameterSerializable snapshotBeforeEdit_;

    boost::signals2::scoped_connection valueChangedConnection_;
};

} // namespace enzo::ui
