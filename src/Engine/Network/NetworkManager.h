#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/UpdateLock.h"
#include "Engine/NetworkGraph/NetworkGraph.h"
#include "Engine/UndoRedo/UndoStack.h"
#include <memory>
#include <unordered_map>

namespace enzo {
class NetworkPath;
}

namespace enzo::nt {
/**
 * @brief The central coordinator of the engine's node system.
 *
 * The network manager is the central coordinator of the engine’s node system.
 * It manages the lifecycle of nodes, including their creation, storage,
 * and validation, while also tracking dependencies between them. Acting
 * as a singleton, it ensures that all parts of the engine work with a single
 * consistent view of the network, providing global access. Beyond just storing
 * nodes, it also controls cooking and traversing dependency graphs,
 * ensuring that updates flow correctly through the network when nodes change.
 */
class NetworkManager
{
  public:
    /// @brief Iterable range over nodes, yields {NodeId, Node&} pairs.
    class NodeRange
    {
        using Map = std::unordered_map<NodeId, std::unique_ptr<Node>>;
        Map& map_;

      public:
        class Iterator
        {
            Map::iterator it_;

          public:
            using value_type = std::pair<const NodeId, Node&>;
            using reference = value_type;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            Iterator(Map::iterator it) : it_(it) {}
            reference operator*() const { return {it_->first, *it_->second}; }
            Iterator& operator++()
            {
                ++it_;
                return *this;
            }
            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++it_;
                return tmp;
            }
            bool operator==(const Iterator& other) const { return it_ == other.it_; }
            bool operator!=(const Iterator& other) const { return it_ != other.it_; }
        };

        NodeRange(Map& map) : map_(map) {}
        Iterator begin() { return Iterator(map_.begin()); }
        Iterator end() { return Iterator(map_.end()); }
        std::size_t size() const { return map_.size(); }
    };

    /// @brief Returns an iterable range over all nodes in the network.
    NodeRange nodes() { return NodeRange(nodeStore_); }

    /// @brief Deleted the copy constructor for singleton.
    NetworkManager(const NetworkManager& obj) = delete;

    /// @brief Returns a reference to the singleton instance.
    static NetworkManager& getInstance();

    /**
     * @brief Creates a new node in the network
     *
     * @param nodeType Data designating the properties of the node.
     * @param path Optional explicit path for the node. When empty an
     *             auto generated path is used. Supplying it here ensures the
     *             path is in place before the nodeCreated signal fires, so
     *             observers see the final name from the start.
     * @param position Where the node sits in the network view.
     *
     * @return The node ID of the newly created node
     *
     * @todo Take the type name rather than the whole node type.
     */
    NodeId createNode(
        const nt::NodeType& nodeType,
        const std::string& path = "",
        Vector2 position = {0.f, 0.f}
    );

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
     * @param nodeId The node to delete.
     */
    void deleteNode(NodeId nodeId);

    /**
     * @brief Removes a node from the network.
     *
     * @note During a delete the connections are owned by their own undo commands, so callers
     * driving an undo or redo pass @p removeConnections as false to remove only the bare node.
     *
     * @param nodeId The node to remove.
     * @param removeConnections When true the node's connections are removed first.
     */
    void removeNode(NodeId nodeId, bool removeConnections = true);

    /**
     * @brief Restores a previously removed node with a specific NodeId.
     *
     * @note This does not restore the state the node was in, only creates a new node with the given
     * id.
     * @todo maybe replace with createNodeWithId
     *
     * @param nodeId The node ID to restore.
     * @param nodeType The node type to restore.
     * @param position The position to restore the node at.
     */
    void restoreNode(NodeId nodeId, const nt::NodeType& nodeType);

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
    nt::NetworkGraph& graph() { return graph_; }

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

    /// @brief Resolves a node reference to its node.
    /// @param path A node path such as "grid_1". An empty node path resolves to @p fromNode.
    /// @param fromNode The node a relative path resolves against, nullNode when there is none.
    /// @return The node, or null when no node matches the path.
    Node* findNode(const NetworkPath& path, NodeId fromNode = nullNode);

    /// @brief Resolves a parameter reference to its parameter.
    /// @param path A parameter path such as "grid_1.tx".
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
    // store for geometry nodes
    std::vector<enzo::nt::NodeId> selectedNodes_;
    std::unordered_map<enzo::nt::NodeId, std::unique_ptr<enzo::nt::Node>> nodeStore_;
    // the highest node id currently stored
    enzo::nt::NodeId maxNodeId_ = 0;
    // node selected for displaying in the viewport
    std::optional<NodeId> displayNode_ = std::nullopt;
    // the primary node that drives the parameter and geometry panes
    std::optional<NodeId> primaryNode_ = std::nullopt;
    // owns the network's wiring and dependencies
    nt::NetworkGraph graph_;

    UndoStack undoStack_;
};

inline enzo::nt::NetworkManager& nm() { return enzo::nt::NetworkManager::getInstance(); }
} // namespace enzo::nt
