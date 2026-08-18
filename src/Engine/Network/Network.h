#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/Scope.h"
#include "Engine/NetworkGraph/NetworkGraph.h"
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace enzo::nt {
/**
 * @brief Every node in a scene, their wiring, and the scopes they live in.
 *
 * This is the whole node graph as one value. It holds data and answers questions
 * about what it holds. Cooking, undo, selection, and signalling all belong to the
 * nt::NetworkManager that owns one of these.
 *
 * Nodes are stored flat and keyed by nt::NodeId, with nesting carried entirely in
 * their paths, so `/container1/grid1` lives in the scope at `/container1` without
 * anything holding it there.
 *
 * @note Not copyable. Each node carries signal connections to the interface, so
 * duplicating a network needs an explicit clone that leaves those behind.
 */
class Network
{
  public:
    using NodeStore = std::unordered_map<NodeId, std::unique_ptr<Node>>;

    Network() { clear(); }

    Network(const Network&) = delete;
    Network& operator=(const Network&) = delete;

    /** @name Nodes
     * @{
     */
    /// @brief Returns an iterable range over every node, yielding {NodeId, Node&} pairs.
    auto nodes()
    {
        return std::views::transform(nodes_, [](NodeStore::value_type& entry) {
            return std::pair<const NodeId, Node&>(entry.first, *entry.second);
        });
    }

    /// @brief Returns the node with the given id.
    /// @note Throws std::out_of_range when no node carries that id.
    Node& getNode(NodeId nodeId);

    /// @brief Returns whether a node with the given id is present.
    bool isValidNode(NodeId nodeId) const;

    /// @brief Returns the node at an exact path, or null when no node is there.
    /// @note Takes a resolved absolute path.
    Node* getNodeAtPath(const Path& path);

    /// @brief Returns the ids of the nodes living directly inside a scope, in no particular order.
    /// @note Nodes deeper inside a nested scope are not included.
    std::vector<NodeId> getChildNodeIds(const Path& scope);

    /// @brief Takes ownership of a node and opens the scope it holds.
    void addNode(NodeId nodeId, std::unique_ptr<Node> node);

    /// @brief Removes a node, the scope it held, and its wiring.
    /// @note The nodes living inside that scope are not dropped with it.
    void eraseNode(NodeId nodeId);

    /// @brief Returns an id no node has used yet.
    NodeId reserveNodeId() { return ++maxNodeId_; }
    /** @} */

    /// @brief Returns the scope at a path, or null when no scope sits there.
    /// @note The root scope always exists, so getScope("/") never returns null.
    Scope* getScope(const Path& path);

    /// @brief Returns the wiring and dependencies between the nodes.
    NetworkGraph& graph() { return graph_; }

    /// @brief Empties the network back to a bare root scope.
    void clear();

  private:
    // A scope exists only because a node holds it, so it opens and closes with that node
    void addScope(const Path& path, const std::string& scopeType);
    void eraseScope(const Path& path);

    // Every node, flat, with nesting carried in their paths
    NodeStore nodes_;
    NetworkGraph graph_;
    // Every scope that exists, keyed by the path it sits at, always holding the root
    std::unordered_map<std::string, Scope> scopes_;
    // The highest node id handed out so far
    NodeId maxNodeId_ = 0;
};
} // namespace enzo::nt
