#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Dependency/Unit.h"
#include <unordered_map>
#include <vector>

namespace enzo::dep {

/**
 * @brief Whether a dependency is known up front or found during a cook.
 *
 * Wired dependencies are physical connections in the node graph. They are known
 * before a cook and drive the cook order. Captured dependencies are expression
 * references seen while a parameter evaluates. They are stored for invalidation
 * and never take part in scheduling.
 */
enum class DependencyKind
{
    Wired,
    Captured
};

/**
 * @brief Directed graph of the dependencies across the network.
 *
 * Every edge points from the value depended upon to the value that reads it, so
 * the dependents of a unit are everything that must update when it changes.
 *
 * @note Wired edges connect whole nodes and are the only ones the cook order
 * considers. Captured edges may start at a node or a parameter and always land
 * on a parameter, joining the wired edges only when working out what to dirty.
 */
class DependencyGraph
{
  public:
    /// @brief Records a connection where @p dependent reads @p dependency.
    void addWiredEdge(nt::OpId dependency, nt::OpId dependent);

    /// @brief Removes a connection between two nodes.
    void removeWiredEdge(nt::OpId dependency, nt::OpId dependent);

    /// @brief Replaces every captured dependency of one parameter at once.
    /// @note A parameter rebuilds its full reference set each time it evaluates,
    /// so its previous captured edges are dropped and the new set takes over.
    void setCapturedDependencies(
        const Unit& dependent,
        const std::vector<Unit>& dependencies
    );

    /// @brief Removes every edge touching the node, as a unit or through any of
    /// its parameters.
    void removeNode(nt::OpId opId);

    /// @brief Empties the graph.
    void clear();

    /// @brief Returns the nodes to cook before @p target, in cook order.
    /// @note Considers wired edges only. Reports a cycle rather than looping.
    std::vector<nt::OpId> cookOrder(nt::OpId target) const;

    /// @brief Returns everything that depends on @p changed, directly or through
    /// a chain.
    /// @note Considers both wired and captured edges.
    std::vector<Unit> dependents(const Unit& changed) const;

  private:
    // The unit an edge reaches and how the dependency was learned.
    struct Edge
    {
        Unit unit;
        DependencyKind kind;
    };

    // Forward adjacency keyed by the depended upon unit listing what reads it.
    // The reverse adjacency lets the cook order walk upstream from a target.
    std::unordered_map<Unit, std::vector<Edge>> dependents_;
    std::unordered_map<Unit, std::vector<Edge>> dependencies_;
};

} // namespace enzo::dep
