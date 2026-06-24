#pragma once
#include "Engine/Parameter/Template.h"
#include "LegacyGui/Parameters/Parameter.h"

namespace enzo::ui {

/// @brief A blank fixed height row that visually separates parameters.
class SpacerParm : public Parameter
{
    Q_OBJECT
  public:
    SpacerParm(const prm::Template& parmTemplate, QWidget* parent = nullptr);
};

} // namespace enzo::ui
