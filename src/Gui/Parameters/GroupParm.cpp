#include "Gui/Parameters/GroupParm.h"

enzo::ui::GroupParm::GroupParm(const prm::Template& parmTemplate, QWidget* parent)
    : Parameter(parmTemplate, parent)
{
}

void enzo::ui::GroupParm::addChild(Parameter* child) { contentLayout_->addWidget(child); }
