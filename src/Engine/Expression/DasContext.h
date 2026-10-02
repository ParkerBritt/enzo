#pragma once
#include "Engine/Core/Types.h"
#include "daScript/daScript.h"
#include <optional>
#include <vector>

namespace enzo::expr {

class ExpressionContext;

/**
 * @brief A daslang runtime context that also carries our evaluation world.
 *
 * daslang hands every native function the context it is running in, so hanging
 * the ExpressionContext here is how prm() and friends reach back into the app
 * without a global. The pointer is set for the span of one evaluation and
 * restored after, since evaluations nest.
 */
struct DasContext : das::Context
{
    DasContext(uint32_t stackSize) : das::Context(stackSize) {}
    DasContext(const DasContext& other, uint32_t category) : das::Context(other, category) {}

    /// @brief The world the running function reads, a ScriptContext when primitiveIndex is set.
    const ExpressionContext* expressionContext = nullptr;

    /// @brief The input primitive the running script works on, empty outside a script run.
    std::optional<size_t> primitiveIndex;

    /// @brief The warnings the runs on this context added, without duplicates.
    std::vector<String> warnings;
};

} // namespace enzo::expr
