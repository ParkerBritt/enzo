#include "Engine/Script/ScriptEngine.h"
#include "Engine/Script/PointScript.h"

namespace enzo::script {

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

} // namespace enzo::script
