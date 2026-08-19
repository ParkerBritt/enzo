#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include <memory>

namespace enzo {

prm::NodeParameter::NodeParameter(Template prmTemplate, nt::NodeId nodeId)
    : Parameter{std::move(prmTemplate)}, nodeId_{nodeId}
{
}

std::unique_ptr<expr::ExpressionContext> prm::NodeParameter::makeExpressionContext_() const
{
    return std::make_unique<expr::ExpressionContext>(nodeId_);
}

void prm::NodeParameter::submitExpressionDependencies_(
    const expr::ExpressionContext& context,
    unsigned int index
) const
{
    nt::nm().graph().setCapturedDependencies(
        nt::Unit{nodeId_, getName(), index},
        context.getExpressionDependencies()
    );
}

} // namespace enzo
