#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ExpressionContext.h"
#include "daScript/ast/ast_interop.h"

// The daslang module that exposes parameter functions to expressions. These
// read live data and return a single value, so they are shared by every
// evaluation context. Geometry and network scripts layer their own functions on
// top of this set later.

namespace enzo::expr {

namespace {

// Reads the evaluation world off the daslang context daslang hands us, null
// when an expression runs with no context behind it.
const ExpressionContext* contextOf(das::Context* dasContext)
{
    return static_cast<DasContext*>(dasContext)->expressionContext;
}

// Returns another parameter's value by path, e.g. prm("grid_1/tx"). Falls back
// to zero when nothing resolves the path.
floatT prm(const char* path, das::Context* dasContext)
{
    const ExpressionContext* context = contextOf(dasContext);
    floatT result = 0;
    if (context) context->evalFloat(path ? path : "", result);
    return result;
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
            *this, lib, "prm", das::SideEffects::modifyExternal, "enzo::expr::prm"
        )
            ->args({"path"});
    }
};

} // namespace enzo::expr

REGISTER_MODULE_IN_NAMESPACE(ParameterModule, enzo::expr);
