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

/// @brief Evaluates a parameter as a float, one component at a time.
///
/// e.g. prm("grid_1.t", 1) reads the second component of a vector, while the
/// index defaults to 0 so prm("grid_1.tx") reads the first.
///
/// @return The float value, or zero when nothing resolves the path.
floatT prm(const char* path, int32_t index, das::Context* dasContext)
{
    auto parameter = parameterAt(path, dasContext);
    return parameter ? parameter->evalFloat(static_cast<unsigned int>(index)) : 0;
}

/// @brief Evaluates a parameter as an integer, e.g. prmI("copies.count").
/// @return The integer value, or zero when nothing resolves the path.
intT prmI(const char* path, int32_t index, das::Context* dasContext)
{
    auto parameter = parameterAt(path, dasContext);
    return parameter ? parameter->evalInt(static_cast<unsigned int>(index)) : 0;
}

/// @brief Evaluates a parameter as a string, e.g. prmS("file.name").
/// @return The string value, or empty when nothing resolves the path.
char* prmS(const char* path, int32_t index, das::Context* dasContext)
{
    auto parameter = parameterAt(path, dasContext);
    const String value = parameter ? parameter->evalString(static_cast<unsigned int>(index)) : "";
    return dasContext->allocateString(value, nullptr);
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
            ->args({"path", "index", "context"})
            ->arg_init(1, new das::ExprConstInt(0));

        das::addExtern<DAS_BIND_FUN(prmI)>(
            *this,
            lib,
            "prmI",
            das::SideEffects::modifyExternal,
            "enzo::expr::prmI"
        )
            ->args({"path", "index", "context"})
            ->arg_init(1, new das::ExprConstInt(0));

        das::addExtern<DAS_BIND_FUN(prmS)>(
            *this,
            lib,
            "prmS",
            das::SideEffects::modifyExternal,
            "enzo::expr::prmS"
        )
            ->args({"path", "index", "context"})
            ->arg_init(1, new das::ExprConstInt(0));
    }
};

} // namespace enzo::expr

REGISTER_MODULE_IN_NAMESPACE(ParameterModule, enzo::expr);
