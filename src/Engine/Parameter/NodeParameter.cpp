#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <memory>

namespace enzo {

prm::NodeParameter::NodeParameter(Template prmTemplate, nt::OpId opId)
    : Parameter{std::move(prmTemplate)}, opId_{opId} {}

void prm::NodeParameter::onFloatSet_(const PrmValues &before) {
    addUndo_(before);
}

void prm::NodeParameter::addUndo_(prm::PrmValues before) {
    auto cmd = std::make_unique<nt::ChangeParameterCommand>(opId_, getName(), before, values_);
    nt::nm().undoStack().push(std::move(cmd));
}

} // namespace enzo
