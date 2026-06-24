#pragma once
#include "Engine/Parameter/Template.h"
#include "LegacyGui/Parameters/Parameter.h"

namespace enzo::ui {

class GroupParm : public Parameter
{
    Q_OBJECT
  public:
    GroupParm(const prm::Template& parmTemplate, QWidget* parent = nullptr);
    void addChild(Parameter* child) override;
};

} // namespace enzo::ui
