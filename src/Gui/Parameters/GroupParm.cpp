#include "Gui/Parameters/GroupParm.h"
#include "Gui/Style.h"

enzo::ui::GroupParm::GroupParm(const prm::Template& parmTemplate, QWidget* parent)
    : Parameter(parmTemplate, parent)
{
    contentLayout_->setSpacing(enzo::style::parameter::gap);
}

void enzo::ui::GroupParm::addChild(Parameter* child) { contentLayout_->addWidget(child); }
