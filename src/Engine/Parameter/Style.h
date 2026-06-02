#pragma once

#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Core/Types.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace enzo::prm::style
{

struct BoolSwitch
{
};

struct BoolIcon
{
    enum Setting { ICON, SCALE };

    std::vector<std::shared_ptr<prm::Parameter>> settings = {
        std::make_shared<prm::Parameter>(Template(Type::STRING, Name("iconName", "Icon Name"), Default("eye"))),
        std::make_shared<prm::Parameter>(Template(Type::FLOAT,  Name("scale",    "Scale"),    Default(0.95f))),
    };

    BoolIcon& setIcon(std::string iconPath)
    {
        settings[ICON]->setString(std::move(iconPath));
        return *this;
    }
    BoolIcon& setScale(floatT scale)
    {
        settings[SCALE]->setFloat(scale);
        return *this;
    }

    String  icon()  const { return settings[ICON]->evalString(); }
    floatT  scale() const { return settings[SCALE]->evalFloat(); }
};

struct BoolIconSlash
{
    enum Setting { ICON, SCALE };

    std::vector<std::shared_ptr<prm::Parameter>> settings = {
        std::make_shared<prm::Parameter>(Template(Type::STRING, Name("iconName", "Icon Name"), Default("eye"))),
        std::make_shared<prm::Parameter>(Template(Type::FLOAT,  Name("scale",    "Scale"),    Default(0.95f))),
    };

    BoolIconSlash& setIcon(std::string iconPath)
    {
        settings[ICON]->setString(std::move(iconPath));
        return *this;
    }
    BoolIconSlash& setScale(floatT scale)
    {
        settings[SCALE]->setFloat(scale);
        return *this;
    }

    String  icon()  const { return settings[ICON]->evalString(); }
    floatT  scale() const { return settings[SCALE]->evalFloat(); }
};

}
