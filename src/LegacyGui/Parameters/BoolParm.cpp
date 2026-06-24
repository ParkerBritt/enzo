#include "LegacyGui/Parameters/BoolParm.h"
#include "Engine/Parameter/Style.h"
#include "LegacyGui/Parameters/BoolIconParm.h"
#include "LegacyGui/Parameters/BoolIconSlashParm.h"
#include "LegacyGui/Parameters/BoolSwitchParm.h"

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
