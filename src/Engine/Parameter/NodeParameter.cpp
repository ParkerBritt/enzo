#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
#include <memory>

namespace enzo {

prm::NodeParameter::NodeParameter(Template prmTemplate, nt::OpId opId)
    : Parameter{std::move(prmTemplate)}, opId_{opId}
{
}

void prm::NodeParameter::onFloatSet_(const PrmValues& before) { addUndo_(before); }

std::unique_ptr<expr::ExpressionContext> prm::NodeParameter::makeExpressionContext_() const
{
    return std::make_unique<expr::ExpressionContext>(opId_);
}

void prm::NodeParameter::submitExpressionDependencies_(
    const expr::ExpressionContext& context,
    unsigned int index
) const
{
    nt::nm().graph().setCapturedDependencies(
        nt::Unit{opId_, getName(), index},
        context.getExpressionDependencies()
    );
}

void prm::NodeParameter::addUndo_(prm::PrmValues before)
{
    auto cmd = std::make_unique<nt::ChangeParameterCommand>(opId_, getName(), before, values_);
    nt::nm().undoStack().push(std::move(cmd));
}

} // namespace enzo
