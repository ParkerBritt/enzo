#pragma once
#include "Engine/Core/Types.h"
#include <memory>
#include <span>
#include <variant>

namespace enzo::expr {

class ExpressionContext;

/// @brief An argument to a script function, either an integer or the address a reference parameter writes to.
using ScriptArgument = std::variant<intT, void*>;

/// @brief The most arguments a script function can take.
inline constexpr size_t maxScriptArguments = 32;

/**
 * @brief A compiled daslang program ready to run.
 *
 * Reused across many evaluations so the cost of compiling is paid once. Each
 * eval helper runs an exported function by name and returns its result.
 *
 * Example
 * @code
 * String error;
 * auto script = DasRuntime::instance().compile("expr", source, error);
 * floatT result = 0;
 * script->evalFloat("__eval__", result, error);
 * @endcode
 *
 * @note daslang types stay inside the implementation so consumers never include
 * daslang headers.
 */
class CompiledScript
{
  public:
    ~CompiledScript();

    /// @brief Evaluates an exported function as a float.
    /// @return True on success, false when the function is missing or panics.
    bool evalFloat(
        const String& functionName,
        const ExpressionContext* context,
        floatT& result,
        String& error
    );

    /// @brief Evaluates an exported function as an integer.
    /// @return True on success, false when the function is missing or panics.
    bool evalInt(
        const String& functionName,
        const ExpressionContext* context,
        intT& result,
        String& error
    );

    /// @brief Evaluates an exported function as a string.
    /// @return True on success, false when the function is missing or panics.
    bool evalString(
        const String& functionName,
        const ExpressionContext* context,
        String& result,
        String& error
    );

    /// @brief Runs an exported function for its side effects.
    /// @return True on success, false when the function is missing or panics.
    bool run(
        const String& functionName,
        std::span<const ScriptArgument> arguments,
        const ExpressionContext* context,
        String& error
    );

    /**
     * @brief Returns a copy that shares the compiled program and runs in its own context.
     *
     * @return The clone, or null when its globals fail to initialise.
     * @note A context runs one function at a time, so each thread needs its own clone.
     */
    std::shared_ptr<CompiledScript> clone() const;

  private:
    friend class DasRuntime;
    struct Impl;

    explicit CompiledScript(Impl impl);

    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Shared host for the daslang runtime.
 *
 * Owns the one time setup of the daslang module system and turns source text
 * into reusable compiled scripts.
 *
 * @note Every daslang consumer builds on this rather than touching daslang
 * directly, so parameter expressions today and geometry script nodes later
 * share one runtime and one place that knows how to compile.
 */
class DasRuntime
{
  public:
    static DasRuntime& instance();

    /**
     * @brief Compiles source text into a reusable script.
     * @return The compiled script, or null when compilation fails.
     * @note On failure the daslang diagnostics are written to @p error.
     */
    std::shared_ptr<CompiledScript>
    compile(const String& name, const String& source, String& error);

    DasRuntime(const DasRuntime&) = delete;
    DasRuntime& operator=(const DasRuntime&) = delete;

  private:
    DasRuntime();
    ~DasRuntime();
};

} // namespace enzo::expr
