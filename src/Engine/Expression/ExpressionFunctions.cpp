#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include "daScript/ast/ast_interop.h"

// The daslang module that exposes enzo's functions to expressions. Each one
// reads live data and returns a single value.

namespace enzo::expr {

namespace {

// Returns the context the running expression belongs to, or null for a preview
// evaluation that has none.
const ExpressionContext* expressionContextOf(das::Context* dasContext)
{
    return static_cast<DasContext*>(dasContext)->expressionContext;
}

// Returns a parameter's value, with the path resolved relative to the node the
// running expression belongs to.
//
// Raises a daslang error when there is no context or the path matches nothing,
// so the failure surfaces as the expression's error.
//
// TODO: when a node error API exists, a failing parameter eval during a node's
// cook should also raise that node's error, not just the expression's.
template <typename Value>
Value readParameter(const char* path, int32_t index, das::Context* dasContext)
{
    const ExpressionContext* context = expressionContextOf(dasContext);
    if (!context) dasContext->throw_error("parameter functions need a node to resolve against");

    const char* safePath = path ? path : "";
    auto value = context->readParameter<Value>(safePath, static_cast<unsigned int>(index));
    if (!value) dasContext->throw_error_ex("no parameter matches path '%s'", safePath);

    return *value;
}

// Parameter functions exposed to daslang expressions

/// @brief Returns a parameter's value as a float, one component at a time.
///
/// e.g. prm("grid_1.t", 1) reads the second component of a vector, while the
/// index defaults to 0 so prm("grid_1.tx") reads the first.
floatT prm(const char* path, int32_t index, das::Context* dasContext)
{
    return readParameter<floatT>(path, index, dasContext);
}

/// @brief Returns a parameter's value as an integer, e.g. prmI("copies.count").
intT prmI(const char* path, int32_t index, das::Context* dasContext)
{
    return readParameter<intT>(path, index, dasContext);
}

/// @brief Returns a parameter's value as a string, e.g. prmS("file.name").
char* prmS(const char* path, int32_t index, das::Context* dasContext)
{
    const String value = readParameter<String>(path, index, dasContext);
    return dasContext->allocateString(value, nullptr);
}

// Time functions exposed to daslang expressions

// Notes on the context that the running expression read the time. A preview
// evaluation has no context and records nothing.
void recordTimeDependency(das::Context* dasContext)
{
    const ExpressionContext* context = expressionContextOf(dasContext);
    if (context) context->recordTimeDependency();
}

/// @brief Returns the frame the scene sits on.
/// @note The frame can be fractional.
floatT frame(das::Context* dasContext)
{
    recordTimeDependency(dasContext);
    return nt::nm().getFrame();
}

/// @brief Returns the scene time in seconds, measured from the start of frame 1.
floatT time(das::Context* dasContext)
{
    recordTimeDependency(dasContext);
    return nt::nm().getTime();
}

} // namespace

class ExpressionModule : public das::Module
{
  public:
    ExpressionModule() : das::Module("enzo_expression")
    {
        das::ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // Marks the functions as reading external mutable state, so daslang does
        // not fold repeated calls to a constant.
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

        das::addExtern<DAS_BIND_FUN(frame)>(
            *this,
            lib,
            "frame",
            das::SideEffects::modifyExternal,
            "enzo::expr::frame"
        )
            ->args({"context"});

        das::addExtern<DAS_BIND_FUN(time)>(
            *this,
            lib,
            "time",
            das::SideEffects::modifyExternal,
            "enzo::expr::time"
        )
            ->args({"context"});
    }
};

} // namespace enzo::expr

REGISTER_MODULE_IN_NAMESPACE(ExpressionModule, enzo::expr);
