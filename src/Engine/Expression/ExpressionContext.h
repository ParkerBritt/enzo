#pragma once
#include "Engine/Core/Types.h"
#include "Engine/NetworkGraph/Unit.h"
#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

namespace enzo::expr {

/**
 * @brief The world a single expression evaluation reads and writes to.
 *
 * One of these lives for the span of one evaluation, and a script run shares one
 * across all its threads.
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
    explicit ExpressionContext(nt::NodeId currentNode) : currentNode_(currentNode) {}

    /// @brief The node a relative parameter path resolves against.
    nt::NodeId currentNode() const { return currentNode_; }

    /**
     * @brief Returns a parameter's value, evaluated on the first read and reused after.
     *
     * @return The value, or nothing when the path matches no parameter.
     * @note The first read records the parameter's node as a dependency.
     * @note Reads from several threads evaluate one at a time.
     */
    template <typename Value>
    std::optional<Value> readParameter(const String& path, unsigned int index) const;

    /// @brief Every parameter the expression read during this evaluation.
    const std::vector<nt::Unit>& getExpressionDependencies() const
    {
        return expressionDependencies_;
    }

    /// @brief Notes that the expression read the scene time.
    void recordTimeDependency() const { readsTime_ = true; }

    /// @brief Returns whether the expression read the scene time during this evaluation.
    bool dependsOnTime() const { return readsTime_; }

  private:
    using ParameterValue = std::variant<floatT, intT, String>;

    // A read's path, component index and value type.
    using ParameterRead = std::tuple<String, unsigned int, size_t>;

    nt::NodeId currentNode_;

    // Filled as prm() and friends resolve, so const reads can still accumulate.
    mutable std::mutex readMutex_;
    mutable std::map<ParameterRead, ParameterValue> parameterValues_;
    mutable std::vector<nt::Unit> expressionDependencies_;
    mutable std::atomic<bool> readsTime_ = false;
};

/// @brief Records everything an expression read as dependencies of the node that ran it.
void submitExpressionDependencies(const ExpressionContext& context);

extern template std::optional<floatT>
ExpressionContext::readParameter<floatT>(const String&, unsigned int) const;
extern template std::optional<intT>
ExpressionContext::readParameter<intT>(const String&, unsigned int) const;
extern template std::optional<String>
ExpressionContext::readParameter<String>(const String&, unsigned int) const;

} // namespace enzo::expr
