#include "Engine/Expression/ScriptEngine.h"
#include "Engine/Expression/PointScript.h"

namespace enzo::expr {

ScriptEngine& ScriptEngine::instance()
{
    static ScriptEngine engine;
    return engine;
}

std::shared_ptr<const PointScript> ScriptEngine::compile(const String& code, String& error)
{
    auto cached = cache_.find(code);
    if (cached != cache_.end()) return cached->second;

    std::shared_ptr<const PointScript> script = PointScript::compile(code, error);
    if (script) cache_[code] = script;
    return script;
}

} // namespace enzo::expr
