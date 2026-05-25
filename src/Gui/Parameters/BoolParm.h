#pragma once

#include "Engine/Parameter/NodeParameter.h"
#include "Gui/Parameters/Parameter.h"
#include <memory>

namespace enzo::ui
{

class BoolParm
{
public:
    static Parameter* create(std::weak_ptr<prm::NodeParameter> parameter, QWidget* parent = nullptr);
};

}
