#pragma once
#include "daScript/daScript.h"

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

    const ExpressionContext* expressionContext = nullptr;
};

} // namespace enzo::expr
