#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NetworkPath.h"
#include "Engine/Parameter/NodeParameter.h"
#include "daScript/ast/ast_interop.h"

// The daslang module that exposes parameter functions to expressions. These
// read live data and return a single value, so they are shared by every
// evaluation context. Geometry and network scripts layer their own functions on
// top of this set later.

namespace enzo::expr {

namespace {

// Returns the parameter a path points at, resolved relative to the node the
// running expression belongs to. daslang hands us that node on the context.
// Empty when there is no context or the path matches nothing.
std::shared_ptr<prm::NodeParameter> parameterAt(const char* path, das::Context* dasContext)
{
    const ExpressionContext* context = static_cast<DasContext*>(dasContext)->expressionContext;
    if (!context) return nullptr;

    auto parameter =
        nt::nm().findParameter(NetworkPath(path ? path : ""), context->currentOp()).lock();

    // Reading a parameter makes its node a dependency, so the expression recooks
    // when that node changes.
    if (parameter) context->recordExpressionDependency(nt::Unit{parameter->getOpId()});

    return parameter;
}

// Parameter functions exposed to daslang expressions

// Returns another parameter's value by path, e.g. prm("grid_1.tx"). Falls back
// to zero when nothing resolves the path.
floatT prm(const char* path, das::Context* dasContext)
{
    auto parameter = parameterAt(path, dasContext);
    return parameter ? parameter->evalFloat() : 0;
}

} // namespace

class ParameterModule : public das::Module
{
  public:
    ParameterModule() : das::Module("enzo_parameter")
    {
        das::ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // prm reads external mutable state, so it must not be folded as a
        // constant across repeated calls.
        das::addExtern<DAS_BIND_FUN(prm)>(
            *this,
            lib,
            "prm",
            das::SideEffects::modifyExternal,
            "enzo::expr::prm"
        )
            ->args({"path", "context"});
    }
};

} // namespace enzo::expr

REGISTER_MODULE_IN_NAMESPACE(ParameterModule, enzo::expr);
