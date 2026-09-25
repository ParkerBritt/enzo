#pragma once
#include "Engine/Core/Types.h"
#include "Engine/NetworkGraph/NodeLink.h"
#include "Engine/NetworkGraph/Unit.h"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enzo::nt {

/**
 * @brief The single owner of the network's wiring and dependencies.
 *
 * Node links are the physical wires between nodes. They are the ground
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
    /// @brief Records a node link between two nodes.
    void connect(const NodeLink& nodeLink);

    /// @brief Removes a node link between two nodes.
    void disconnect(const NodeLink& nodeLink);

    /// @brief Returns the node links feeding @p target, ordered by input index.
    std::vector<NodeLink> getInputs(NodeId target) const;

    /// @brief Returns the node link on one input of @p target, if any.
    /// @note An input holds at most one node link.
    std::optional<NodeLink> getInputNodeLink(NodeId target, unsigned int inputIndex) const;

    /// @brief Returns the node links leaving @p source.
    std::vector<NodeLink> getOutputs(NodeId source) const;

    /// @brief Returns every node link in the graph, in no particular order.
    std::vector<NodeLink> getNodeLinks() const;

    /// @brief Replaces every captured dependency of one parameter at once.
    /// @note A parameter rebuilds its full reference set each time it evaluates,
    /// so its previous captured edges are dropped and the new set takes over.
    void setCapturedDependencies(const Unit& dependent, const std::vector<Unit>& dependencies);

    /// @brief Records whether a unit reads the scene time.
    /// @note A parameter records this each time it evaluates and a node each
    /// time it cooks.
    void setTimeDependent(const Unit& dependent, bool dependsOnTime);

    /// @brief Returns every unit the scene time reaches, in no particular order.
    /// @note Covers the units that read the time and everything downstream of them.
    std::vector<Unit> getTimeDependents() const;

    /// @brief Removes every node link and captured edge touching the node.
    void removeNode(NodeId nodeId);

    /// @brief Empties the graph.
    void clear();

    /// @brief Returns the nodes to cook before @p target, in cook order.
    /// @note Considers node links only. Reports a cycle rather than looping.
    std::vector<NodeId> getCookOrder(NodeId target) const;

    /// @brief Returns everything that depends on @p changed, directly or through
    /// a chain.
    /// @note Considers both node links and captured edges.
    std::vector<Unit> getDependents(const Unit& changed) const;

  private:
    using NodeLinkMap = std::unordered_map<NodeId, std::vector<NodeLink>>;
    using CapturedMap = std::unordered_map<Unit, std::vector<Unit>>;

    /// @brief Erases one matching node link from a single node's list.
    static void eraseNodeLink_(NodeLinkMap& side, NodeId key, const NodeLink& nodeLink);

    /// @brief Drops every node link that names the node, on a single side.
    static void eraseNodeLinksTouching_(NodeLinkMap& side, NodeId nodeId);

    /// @brief Drops every captured edge that names the node, on a single map.
    static void eraseCapturedTouching_(CapturedMap& map, NodeId nodeId);

    /// @brief Removes @p value from the list stored under @p key.
    static void eraseUnit_(CapturedMap& map, const Unit& key, const Unit& value);

    /// @brief Records @p unit as a dependent unless it was already reached.
    static void addDependent_(
        const Unit& unit,
        std::vector<Unit>& dependents,
        std::unordered_set<Unit>& seen,
        std::vector<Unit>& pending
    );

    /// @brief Adds @p nodeId to @p nodeOrder after its wired dependencies.
    void addToCookOrder_(
        NodeId nodeId,
        std::vector<NodeId>& nodeOrder,
        std::unordered_set<NodeId>& addedNodes,
        std::unordered_set<NodeId>& nodesBeingAdded
    ) const;

    // Input node links keyed by the downstream node.
    NodeLinkMap byTarget_;
    // Output node links keyed by the upstream node.
    NodeLinkMap bySource_;
    // Captured edges are mixed granularity. The source is stored at node level
    // since dirtying is node wide, while the reader is stored per parameter
    // component so re-evaluating one component leaves the others intact.
    // getDependents collapses the reader back to its node to bridge the two.

    // Captured readers keyed by the node they read.
    CapturedMap capturedDependents_;
    // Captured reads keyed by the reading parameter component.
    CapturedMap capturedDependencies_;

    // The units that read the scene time, at the same mixed granularity as the
    // captured maps.
    std::unordered_set<Unit> timeDependents_;
};

} // namespace enzo::nt
