#pragma once

#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Types.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace enzo::prm::style
{

struct BoolSwitch
{
};

struct BoolCrossButton
{
    enum Setting { ICON, SCALE };

    std::vector<std::shared_ptr<prm::Parameter>> settings = {
        std::make_shared<prm::Parameter>(Template(Type::STRING, Name("iconPath", "Icon Path"), Default("x.svg"))),
        std::make_shared<prm::Parameter>(Template(Type::FLOAT,  Name("scale",    "Scale"),    Default(1.0f))),
    };

    BoolCrossButton& setIcon(std::string iconPath)
    {
        settings[ICON]->setString(std::move(iconPath));
        return *this;
    }
    BoolCrossButton& setScale(bt::floatT scale)
    {
        settings[SCALE]->setFloat(scale);
        return *this;
    }

    bt::String  icon()  const { return settings[ICON]->evalString(); }
    bt::floatT  scale() const { return settings[SCALE]->evalFloat(); }
};

}
