#include "Engine/Expression/ExpressionEngine.h"
#include "Engine/Expression/DasRuntime.h"

namespace enzo::expr {

namespace {
constexpr const char* evalFunctionName = "enzoEvalExpression";

// Wraps a bare expression in an exported function that returns the given type,
// so daslang has a function to compile and call.
String wrapExpression(const String& expression, const String& returnType)
{
    return "options gen2\n"
           "require math\n"
           "require enzo_parameter\n"
           "[export]\n"
           "def " +
           String(evalFunctionName) + " : " + returnType +
           " {\n"
           "    return " +
           returnType + "(" + expression +
           ")\n"
           "}\n";
}
} // namespace

ExpressionEngine& ExpressionEngine::instance()
{
    static ExpressionEngine engine;
    return engine;
}

std::shared_ptr<CompiledScript>
ExpressionEngine::compileCached_(const String& source, String& error)
{
    auto cached = cache_.find(source);
    if (cached != cache_.end()) return cached->second;

    auto script = DasRuntime::instance().compile("expression", source, error);
    if (script) cache_[source] = script;
    return script;
}

bool ExpressionEngine::evalFloat(
    const String& expression,
    const ExpressionContext* context,
    floatT& result,
    String& error
)
{
    auto script = compileCached_(wrapExpression(expression, "float"), error);
    if (!script) return false;
    return script->evalFloat(evalFunctionName, context, result, error);
}

bool ExpressionEngine::evalInt(
    const String& expression,
    const ExpressionContext* context,
    intT& result,
    String& error
)
{
    auto script = compileCached_(wrapExpression(expression, "int64"), error);
    if (!script) return false;
    return script->evalInt(evalFunctionName, context, result, error);
}

bool ExpressionEngine::evalString(
    const String& expression,
    const ExpressionContext* context,
    String& result,
    String& error
)
{
    auto script = compileCached_(wrapExpression(expression, "string"), error);
    if (!script) return false;
    return script->evalString(evalFunctionName, context, result, error);
}

} // namespace enzo::expr
