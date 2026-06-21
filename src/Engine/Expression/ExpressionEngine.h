#pragma once
#include "Engine/Core/Types.h"
#include <memory>
#include <unordered_map>

namespace enzo::expr {

class CompiledScript;
class ExpressionContext;

/**
 * @brief Evaluator for parameter expressions written in daslang.
 *
 * A bare expression such as "5 + 5" is wrapped in a function, compiled through
 * DasRuntime, and run to produce a parameter value.
 *
 * @note Compiled expressions are cached so repeated evaluation during cooking
 * stays cheap.
 */
class ExpressionEngine
{
  public:
    static ExpressionEngine& instance();

    /// @brief Evaluates an expression and returns its result as a float.
    /// @param context The world the expression reads, e.g. for prm(), may be null.
    /// @return True on success, false when the expression fails to compile or run.
    bool evalFloat(
        const String& expression,
        const ExpressionContext* context,
        floatT& result,
        String& error
    );

    /// @brief Evaluates an expression and returns its result as an integer.
    /// @param context The world the expression reads, e.g. for prm(), may be null.
    /// @return True on success, false when the expression fails to compile or run.
    bool evalInt(
        const String& expression,
        const ExpressionContext* context,
        intT& result,
        String& error
    );

    /// @brief Evaluates an expression and returns its result as a string.
    /// @param context The world the expression reads, e.g. for prm(), may be null.
    /// @return True on success, false when the expression fails to compile or run.
    bool evalString(
        const String& expression,
        const ExpressionContext* context,
        String& result,
        String& error
    );

    ExpressionEngine(const ExpressionEngine&) = delete;
    ExpressionEngine& operator=(const ExpressionEngine&) = delete;

  private:
    ExpressionEngine() = default;
    std::shared_ptr<CompiledScript> compileCached_(const String& source, String& error);

    std::unordered_map<String, std::shared_ptr<CompiledScript>> cache_;
};

} // namespace enzo::expr
