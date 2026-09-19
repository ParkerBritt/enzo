#pragma once
#include "Engine/Core/Types.h"
#include <memory>
#include <unordered_map>

namespace enzo::expr {

class CompiledScript;

/**
 * @brief Compiler for the per point scripts a script node runs.
 *
 * The user's code is the body of a function that takes the current point offset
 * as `pt`, and runs once per point.
 *
 * @code
 * String error;
 * auto script = ScriptEngine::instance().compile("print(\"{pt}\")", error);
 * const ScriptArgument arguments[] = {intT(0)};
 * script->run(ScriptEngine::runFunctionName, arguments, &context, error);
 * @endcode
 *
 * @note Compiled scripts are cached by their code.
 */
class ScriptEngine
{
  public:
    static ScriptEngine& instance();

    /// @brief The function a compiled script runs once per point.
    static constexpr const char* runFunctionName = "enzoRunScript";

    /// @brief Returns the compiled script for the user's code.
    /// @return The compiled script, or null when the code fails to compile.
    std::shared_ptr<CompiledScript> compile(const String& code, String& error);

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

  private:
    ScriptEngine() = default;

    std::unordered_map<String, std::shared_ptr<CompiledScript>> cache_;
};

} // namespace enzo::expr
