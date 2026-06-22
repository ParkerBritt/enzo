#pragma once
#include "Engine/Core/Types.h"
#include "Engine/NetworkGraph/Connection.h"
#include "Engine/NetworkGraph/Unit.h"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enzo::nt {

/**
 * @brief The single owner of the network's wiring and dependencies.
 *
 * Wired connections are the physical links between nodes. They are the ground
 * truth of the topology and the only edges the cook order considers. Captured
 * dependencies are expression references seen while a parameter evaluates. They
 * are stored for invalidation and never take part in scheduling.
 *
 * Every edge points from the value depended upon to the value that reads it, so
 * the dependents of a unit are everything that must update when it changes.
 */
class NetworkGraph
{
  public:
    /// @brief Records a wired connection between two nodes.
    void connect(const Connection& connection);

    /// @brief Removes a wired connection between two nodes.
    void disconnect(const Connection& connection);

    /// @brief Returns the connections feeding @p target, ordered by input slot.
    std::vector<Connection> getInputs(OpId target) const;

    /// @brief Returns the connection on one input slot of @p target, if any.
    /// @note An input slot holds at most one connection.
    std::optional<Connection> getInputConnection(OpId target, unsigned int inputSlot) const;

    /// @brief Returns the connections leaving @p source.
    std::vector<Connection> getOutputs(OpId source) const;

    /// @brief Replaces every captured dependency of one parameter at once.
    /// @note A parameter rebuilds its full reference set each time it evaluates,
    /// so its previous captured edges are dropped and the new set takes over.
    void setCapturedDependencies(const Unit& dependent, const std::vector<Unit>& dependencies);

    /// @brief Removes every connection and captured edge touching the node.
    void removeNode(OpId opId);

    /// @brief Empties the graph.
    void clear();

    /// @brief Returns the nodes to cook before @p target, in cook order.
    /// @note Considers wired connections only. Reports a cycle rather than looping.
    std::vector<OpId> getCookOrder(OpId target) const;

    /// @brief Returns everything that depends on @p changed, directly or through
    /// a chain.
    /// @note Considers both wired connections and captured edges.
    std::vector<Unit> getDependents(const Unit& changed) const;

  private:
    using ConnectionMap = std::unordered_map<OpId, std::vector<Connection>>;
    using CapturedMap = std::unordered_map<Unit, std::vector<Unit>>;

    /// @brief Erases one matching connection from a single node's list.
    static void eraseConnection_(ConnectionMap& side, OpId key, const Connection& connection);

    /// @brief Drops every connection that names the node, on a single side.
    static void eraseConnectionsTouching_(ConnectionMap& side, OpId opId);

    /// @brief Drops every captured edge that names the node, on a single map.
    static void eraseCapturedTouching_(CapturedMap& map, OpId opId);

    /// @brief Removes @p value from the list stored under @p key.
    static void eraseUnit_(CapturedMap& map, const Unit& key, const Unit& value);

    /// @brief Records @p unit as a dependent unless it was already reached.
    static void addDependent_(
        const Unit& unit,
        std::vector<Unit>& dependents,
        std::unordered_set<Unit>& seen,
        std::vector<Unit>& pending
    );

    /// @brief Adds @p opId to @p opOrder after its wired dependencies.
    void addToCookOrder_(
        OpId opId,
        std::vector<OpId>& opOrder,
        std::unordered_set<OpId>& addedOps,
        std::unordered_set<OpId>& opsBeingAdded
    ) const;

    // Input connections keyed by the downstream node.
    ConnectionMap byTarget_;
    // Output connections keyed by the upstream node.
    ConnectionMap bySource_;
    // Captured edges are mixed granularity. The source is stored at node level
    // since dirtying is node wide, while the reader is stored per parameter
    // component so re-evaluating one component leaves the others intact.
    // getDependents collapses the reader back to its node to bridge the two.

    // Captured readers keyed by the node they read.
    CapturedMap capturedDependents_;
    // Captured reads keyed by the reading parameter component.
    CapturedMap capturedDependencies_;
};

} // namespace enzo::nt
