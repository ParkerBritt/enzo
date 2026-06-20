#pragma once
#include "Engine/Core/Types.h"
#include "Engine/NetworkGraph/Unit.h"

namespace enzo::nt {

/**
 * @brief The world a node parameter's expression reads while it evaluates.
 *
 * An expression carries one of these so parameter functions like prm() can
 * reach back into the live network. A path names another node and one of its
 * parameters, e.g. "grid_1/tx", while a bare name reads a parameter on the node
 * the expression belongs to.
 *
 * Lifetime: built fresh for one evaluation and discarded when it finishes, so
 * it always sees the network as it stands at eval time.
 */
class ExpressionContext
{
  public:
    explicit ExpressionContext(OpId currentOp) : currentOp_(currentOp) {}

    /// @brief Reads another parameter's value by path.
    /// @return True when the path resolves, false otherwise.
    bool evalFloat(const String& path, floatT& result) const;

    /// @brief Reads another parameter's value by path as an integer.
    /// @return True when the path resolves, false otherwise.
    bool evalInt(const String& path, intT& result) const;

  private:
    OpId currentOp_;
};

} // namespace enzo::nt
