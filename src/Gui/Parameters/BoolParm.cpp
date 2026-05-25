#include "Gui/Parameters/BoolParm.h"
#include "Engine/Parameter/Style.h"
#include "Gui/Parameters/BoolCrossButtonParm.h"
#include "Gui/Parameters/BoolSwitchParm.h"

enzo::ui::Parameter* enzo::ui::BoolParm::create(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
{
    auto parameterShared = parameter.lock();
    if (!parameterShared) return nullptr;

    const std::any& style = parameterShared->getTemplate().getStyle();
    if (auto crossButton = std::any_cast<std::shared_ptr<prm::style::BoolCrossButton>>(&style))
    {
        return new BoolCrossButtonParm(parameter, *crossButton, parent);
    }
    return new BoolSwitchParm(parameter, parent);
}
