#pragma once
#include "Engine/Core/Types.h"
#include <memory>

namespace enzo::expr {

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

    /// @brief Runs an exported function and returns its result as a float.
    /// @return True on success, false when the function is missing or panics.
    bool evalFloat(const String& functionName, floatT& result, String& error);

    /// @brief Runs an exported function and returns its result as an integer.
    /// @return True on success, false when the function is missing or panics.
    bool evalInt(const String& functionName, intT& result, String& error);

  private:
    friend class DasRuntime;
    CompiledScript();

    struct Impl;
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
