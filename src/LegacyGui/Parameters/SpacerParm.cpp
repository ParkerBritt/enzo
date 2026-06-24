#include "LegacyGui/Parameters/SpacerParm.h"
#include "LegacyGui/Style.h"

enzo::ui::SpacerParm::SpacerParm(const prm::Template& parmTemplate, QWidget* parent)
    : Parameter(parmTemplate, parent)
{
    setFixedHeight(enzo::style::spacer::height);
}
