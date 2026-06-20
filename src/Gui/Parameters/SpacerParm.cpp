#include "Gui/Parameters/SpacerParm.h"
#include "Gui/Style.h"

enzo::ui::SpacerParm::SpacerParm(const prm::Template& parmTemplate, QWidget* parent)
    : Parameter(parmTemplate, parent)
{
    setFixedHeight(spacerHeight);
}
