#pragma once
#include "Engine/Core/Types.h"
#include <memory>
#include <unordered_map>

namespace enzo::expr {

class PointScript;

/**
 * @brief Compiler for the per point scripts a script node runs.
 *
 * @code
 * String error;
 * auto script = ScriptEngine::instance().compile("@height = pt", error)->clone();
 * script->addWrittenAttributes(output, error);
 * script->run(input, output, 0, input.getNumPoints(), &context, error);
 * @endcode
 *
 * @note Compiled scripts are cached by their code.
 */
class ScriptEngine
{
  public:
    static ScriptEngine& instance();

    /// @brief Returns the compiled script for the user's code.
    /// @return The compiled script, or null when the code fails to compile.
    std::shared_ptr<const PointScript> compile(const String& code, String& error);

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

  private:
    ScriptEngine() = default;

    std::unordered_map<String, std::shared_ptr<const PointScript>> cache_;
};

} // namespace enzo::expr
