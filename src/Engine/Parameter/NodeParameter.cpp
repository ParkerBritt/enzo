#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <memory>

enzo::prm::NodeParameter::NodeParameter(Template prmTemplate, enzo::nt::OpId opId)
    : Parameter{std::move(prmTemplate)}, opId_{opId} {}

void enzo::prm::NodeParameter::onFloatSet_(const PrmValues &before) {
    addUndo_(before);
}

void enzo::prm::NodeParameter::addUndo_(enzo::prm::PrmValues before) {
    auto cmd = std::make_unique<enzo::nt::ChangeParameterCommand>(opId_, getName(), before, values_);
    enzo::nt::nm().undoStack().push(std::move(cmd));
}
