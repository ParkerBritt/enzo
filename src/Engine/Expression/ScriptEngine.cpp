#include "Engine/Expression/ScriptEngine.h"
#include "Engine/Expression/DasRuntime.h"

namespace enzo::expr {

namespace {
// Returns the user's code wrapped in an exported function that takes the point offset.
String wrapScript(const String& code)
{
    return "options gen2\n"
           "require math\n"
           "require enzo_expression\n"
           "[export]\n"
           "def " +
           String(ScriptEngine::runFunctionName) +
           "(pt : int64) {\n" + code +
           "\n"
           "}\n";
}
} // namespace

ScriptEngine& ScriptEngine::instance()
{
    static ScriptEngine engine;
    return engine;
}

std::shared_ptr<CompiledScript> ScriptEngine::compile(const String& code, String& error)
{
    auto cached = cache_.find(code);
    if (cached != cache_.end()) return cached->second;

    auto script = DasRuntime::instance().compile("script", wrapScript(code), error);
    if (script) cache_[code] = script;
    return script;
}

} // namespace enzo::expr
