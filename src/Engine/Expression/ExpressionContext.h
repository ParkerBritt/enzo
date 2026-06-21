#pragma once
#include "Engine/Core/Types.h"

namespace enzo::expr {

/**
 * @brief The node an expression is evaluating against.
 *
 * Parameter functions like prm() take a path that may be relative, so they need
 * to know which node the expression belongs to. An expression carries one of
 * these for the span of a single evaluation.
 */
class ExpressionContext
{
  public:
    explicit ExpressionContext(nt::OpId currentOp) : currentOp_(currentOp) {}

    /// @brief The node a relative parameter path resolves against.
    nt::OpId currentOp() const { return currentOp_; }

  private:
    nt::OpId currentOp_;
};

} // namespace enzo::expr
