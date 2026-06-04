#include "Gui/Parameters/BoolParm.h"
#include "Engine/Parameter/Style.h"
#include "Gui/Parameters/BoolIconParm.h"
#include "Gui/Parameters/BoolIconSlashParm.h"
#include "Gui/Parameters/BoolSwitchParm.h"

enzo::ui::Parameter*
enzo::ui::BoolParm::create(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent)
{
    auto parameterShared = parameter.lock();
    if (!parameterShared) return nullptr;

    const std::any& style = parameterShared->getTemplate().getStyle();
    if (auto icon = std::any_cast<std::shared_ptr<prm::style::BoolIcon>>(&style))
    {
        return new BoolIconParm(parameter, *icon, parent);
    }
    if (auto iconSlash = std::any_cast<std::shared_ptr<prm::style::BoolIconSlash>>(&style))
    {
        return new BoolIconSlashParm(parameter, *iconSlash, parent);
    }
    return new BoolSwitchParm(parameter, parent);
}
