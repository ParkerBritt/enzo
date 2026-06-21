#pragma once
#include "Engine/Core/Types.h"
#include "Engine/NetworkGraph/Unit.h"
#include <vector>

namespace enzo::expr {

/**
 * @brief The world a single expression evaluation reads and writes to.
 *
 * One of these lives for the span of one evaluation.
 *
 * Parameter functions like prm() take a path that may be relative, so they need
 * to know which node the expression belongs to.
 *
 * It also keeps track of dependencies, like other parameters. Once all the
 * dependencies are known they are passed to the network graph at once, so it can
 * use them for tracking updates.
 */
class ExpressionContext
{
  public:
    explicit ExpressionContext(nt::OpId currentOp) : currentOp_(currentOp) {}

    /// @brief The node a relative parameter path resolves against.
    nt::OpId currentOp() const { return currentOp_; }

    /// @brief Notes a parameter the expression read, so it becomes a dependency.
    void recordExpressionDependency(const nt::Unit& dependency) const
    {
        expressionDependencies_.push_back(dependency);
    }

    /// @brief Every parameter the expression read during this evaluation.
    const std::vector<nt::Unit>& getExpressionDependencies() const
    {
        return expressionDependencies_;
    }

  private:
    nt::OpId currentOp_;

    // Filled as prm() and friends resolve, so const reads can still accumulate.
    mutable std::vector<nt::Unit> expressionDependencies_;
};

} // namespace enzo::expr
