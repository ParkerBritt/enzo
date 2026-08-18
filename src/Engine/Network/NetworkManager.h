#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Network/Network.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/Scope.h"
#include "Engine/Network/UpdateLock.h"
#include "Engine/NetworkGraph/NetworkGraph.h"
#include "Engine/UndoRedo/UndoStack.h"
#include <optional>
#include <vector>

namespace enzo {
class NetworkPath;
}

namespace enzo::nt {
/**
 * @brief The central coordinator of the engine's node system.
 *
 * The manager owns one nt::Network and drives everything that happens to it. The
 * network holds the nodes, the wiring, and the scopes, while the manager owns the
 * lifecycle around them, so creating and deleting nodes, cooking, undo, selection,
 * and the signals the interface listens to.
 *
 * Keeping the network a plain value means it can be handed straight to something
 * that only needs to read it, such as the serializer.
 *
 * @note A singleton, so every part of the engine works against one network.
 */
class NetworkManager
{
  public:
    /// @brief Returns the network holding every node, its wiring, and its scopes.
    nt::Network& network() { return network_; }

    /// @brief Returns an iterable range over all nodes in the network.
    auto nodes() { return network_.nodes(); }

    /// @brief Deleted the copy constructor for singleton.
    NetworkManager(const NetworkManager& obj) = delete;

    /// @brief Returns a reference to the singleton instance.
    static NetworkManager& getInstance();

    /**
     * @brief Creates a new node inside a scope.
     *
     * A node left unnamed takes the type name followed by the first free number, so the
     * first grid placed in a scope becomes "grid1" and the next becomes "grid2". Only
     * siblings have to differ, so each scope numbers its own nodes.
     *
     * @note The name is in place before the nodeCreated signal fires, so observers see the
     * final name from the start.
     *
     * @param nodeType Data designating the properties of the node.
     * @param parent The scope to create the node inside. The root holds the top level nodes.
     * @param name The name to give the node. Left empty a free one is picked, and a name
     * already taken by a sibling has a number appended until it is free.
     * @param position Where the node sits in the network view.
     *
     * @return The node ID of the newly created node
     *
     * @todo Take the type name rather than the whole node type.
     */
    NodeId createNode(
        const nt::NodeType& nodeType,
        const Path& parent = Path("/"),
        const std::string& name = "",
        Vector2 position = {0.f, 0.f}
    );

    /// @brief Returns the node at an exact path, or null when no node is there.
    /// @note Takes a resolved absolute path. findNode resolves a reference into one.
    Node* getNodeAtPath(const Path& path) { return network_.getNodeAtPath(path); }

    /// @brief Returns the scope at a path, or null when no scope sits there.
    /// @note The root scope always exists, so getScope("/") never returns null.
    Scope* getScope(const Path& path) { return network_.getScope(path); }

    /// @brief Returns the ids of the nodes living directly inside a scope, in no particular order.
    /// @note Nodes deeper inside a nested scope are not included.
    std::vector<NodeId> getChildNodeIds(const Path& scope)
    {
        return network_.getChildNodeIds(scope);
    }

    /** @brief Returns the node ID for the node with its display flag set.
     * There can only be only be one node displayed at a time.
     * Return value is nullopt if no node is set to display
     */
    std::optional<NodeId> getDisplayNode();

    /** @brief Creates a lock object that prevents cooking until destroyed
     */
    enzo::nt::UpdateLock lockUpdates();

    /**
     * @brief Cooks dirtied nodes, is called automatically
     *
     */
    void update();

    /**
     * @brief Returns whether the node exists in the network and is valid.
     * @param nodeId Node ID of the node to check the validity of.
     */
    bool isValidNode(nt::NodeId nodeId);

    /**
     * @brief Returns a reference to the Node with the given NodeId
     */
    Node& getNode(nt::NodeId nodeId);

    /**
     * @brief Sets given NodeId to be displayed, releasing previous display Node
     */
    void setDisplayNode(NodeId nodeId);

    /**
     * @brief Clears the display flag so no node is displayed
     */
    void clearDisplayFlag();

    /** @brief Returns the node ID of the primary node, or nullopt when none.
     *
     * The primary node is the single node that drives the parameter and geometry
     * panes. There can only be one at a time. It persists when the selection is
     * cleared, unlike the selection itself.
     */
    std::optional<NodeId> getPrimaryNode();

    /**
     * @brief Sets the given NodeId as the primary node, releasing the previous one.
     */
    void setPrimaryNode(NodeId nodeId);

    /**
     * @brief Clears the primary node so none is primary.
     */
    void clearPrimaryNode();

    /**
     * @brief Set the selection state for the given node.
     *
     * @param nodeId The node to set the state on.
     * @param selected The selection state, true selects the node, false unselects it.
     * @param add By default all other nodes are unselected, this parameter
     * allows adding a selected node without deslecting any others.
     */
    void setSelectedNode(NodeId nodeId, bool selected, bool add = false);

    /**
     * @brief Returns the NodeIds for all selected nodes.
     */
    const std::vector<enzo::nt::NodeId>& getSelectedNodes();

    /**
     * @brief Replaces the entire selection with the given set of nodes.
     */
    void setSelectedNodes(std::vector<enzo::nt::NodeId> nodeIds);

    /**
     * @brief Moves a node to a new position, pushing an undo command.
     * @param nodeId The node to move.
     * @param newPos The new position.
     *
     * @todo remove skipUndo argument in favour of a global undo RAII lock
     */
    void moveNode(NodeId nodeId, Vector2 newPos, bool skipUndo = false);

    /**
     * @brief Deletes a node, pushing an undo command.
     *
     * @note A node holding a scope takes the nodes living inside it along, and the whole
     * removal undoes as one step.
     *
     * @param nodeId The node to delete.
     */
    void deleteNode(NodeId nodeId);

    /**
     * @brief Creates a node with an identity the caller dictates rather than one picked here.
     *
     * Every node enters the network here. createNode picks a free id and name and calls this,
     * while undo brings a deleted node back with the id and path it had, since expressions
     * reference nodes by name and a node returning under a new name would break them.
     *
     * @note Throws std::out_of_range when no scope sits at the path's parent.
     *
     * @note Only the node itself is created, not the parameter values or connections it had.
     * The undo commands restore those around this call.
     *
     * @param nodeId The node ID to give the node.
     * @param nodeType The type of node to create.
     * @param path The path to place the node at, whose leaf is the node name.
     * @param position Where the node sits in the network view.
     */
    void createNodeWithId(
        NodeId nodeId,
        const nt::NodeType& nodeType,
        const Path& path,
        Vector2 position
    );

    /**
     * @brief Clears all nodes and resets the network to its initial state.
     */
    void clear();

    /**
     * @brief Cooks the given node
     * @param nodeId node ID to cook
     */
    void cook(enzo::nt::NodeId nodeId);

    /// @brief Returns the graph that owns the network's wiring and dependencies.
    nt::NetworkGraph& graph() { return network_.graph(); }

    /// @brief Wires one node's output into another node's input.
    /// @return The connection that was created.
    /// @note Replaces any connection already on the target input slot.
    nt::Connection connectNodes(
        NodeId inputNodeId,
        unsigned int inputIndex,
        NodeId outputNodeId,
        unsigned int outputIndex
    );

    /// @brief Removes a wired connection between two nodes.
    void disconnectNodes(const nt::Connection& connection);

    /**
     * @brief Resolves a node reference to its node.
     *
     * A relative path is read from the scope holding @p fromNode, so a bare name finds a
     * sibling and ".." steps out to the scope above.
     *
     * @param path A node path such as "grid1", "../grid1", or "/grid1". An empty node path
     * resolves to @p fromNode itself.
     * @param fromNode The node a relative path resolves against, nullNode when there is none.
     * @return The node, or null when no node matches the path.
     */
    Node* findNode(const NetworkPath& path, NodeId fromNode = nullNode);

    /// @brief Resolves a parameter reference to its parameter.
    /// @param path A parameter path such as "grid1.tx".
    /// @param fromNode The node a path with no node part resolves against, nullNode when there is
    /// none.
    /// @return The parameter, or an empty handle when nothing matches.
    std::weak_ptr<prm::NodeParameter>
    findParameter(const NetworkPath& path, NodeId fromNode = nullNode);

    /** @name Signals
     * @{
     */
    // @brief A signal emitted when the display node is changed
    boost::signals2::signal<void(std::optional<nt::NodeId>)> displayNodeChanged;

    // @brief A signal emitted when the geometry to be displayed is changed
    // This is different to #displayNodeChanged because the state of geometry
    // in a node can change based on parameters or other factors.
    boost::signals2::signal<void(std::shared_ptr<const enzo::NodePacket>)> displayGeoChanged;

    // @brief A signal emitted when the selected node's geometry is changed
    boost::signals2::signal<void(std::shared_ptr<const enzo::NodePacket>)> selectedGeoChanged;

    // @brief A signal emitted when the primary node changes
    boost::signals2::signal<void(std::optional<nt::NodeId>)> primaryNodeChanged;

    // @brief A signal emitted when the primary node's geometry changes
    boost::signals2::signal<void(std::shared_ptr<const enzo::NodePacket>)> primaryGeoChanged;

    // @brief A signal emitted when the selection of nodes changes
    boost::signals2::signal<void(std::vector<nt::NodeId> selectedNodeIds)> selectedNodesChanged;

    // @brief A signal emitted when a new node is created in the network
    boost::signals2::signal<void(nt::NodeId)> nodeCreated;

    // @brief A signal emitted when a node is about to be removed from the network
    boost::signals2::signal<void(nt::NodeId)> nodeRemoved;

    // @brief A signal emitted when a connection is created between two nodes
    boost::signals2::signal<void(nt::Connection)> connectionCreated;

    // @brief A signal emitted when a connection is removed between two nodes
    boost::signals2::signal<void(nt::Connection)> connectionRemoved;

    // @brief A signal emitted when the network is cleared
    boost::signals2::signal<void()> networkCleared;

    // @brief A signal emitted when a node's position changes programmatically (e.g. undo/redo)
    boost::signals2::signal<void(nt::NodeId, Vector2)> nodePositionChanged;
    /** @} */

    UndoStack& undoStack() { return undoStack_; }

    /// @brief For use in unit tests, resets the state of the node.
    /// @todo Find a cleaner way to give tests a fresh manager so this
    /// doesn't pollute the public functions.
    void _reset();

  private:
    NetworkManager() {};

    // functions
    /// @brief Removes all of a node's connections, each as its own undo command.
    void disconnectNode(NodeId nodeId);

    /**
     * @brief Slot called when a node of @p NodeId is dirtied
     */
    void onNodeDirtied(nt::NodeId nodeId, bool dirtyDependents);

    // variables
    // every node, its wiring, and the scopes they live in
    nt::Network network_;
    std::vector<enzo::nt::NodeId> selectedNodes_;
    // node selected for displaying in the viewport
    std::optional<NodeId> displayNode_ = std::nullopt;
    // the primary node that drives the parameter and geometry panes
    std::optional<NodeId> primaryNode_ = std::nullopt;

    UndoStack undoStack_;
};

inline enzo::nt::NetworkManager& nm() { return enzo::nt::NetworkManager::getInstance(); }
} // namespace enzo::nt
