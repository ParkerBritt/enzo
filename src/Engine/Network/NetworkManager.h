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
     * A node left unnamed takes its type's name without the namespace followed by the first
     * free number, so the first "enzo::grid" placed in a scope becomes "grid1" and the next
     * becomes "grid2". Only siblings have to differ, so each scope numbers its own nodes.
     *
     * @throws std::runtime_error when no node type or alias is registered under typeName.
     *
     * @param typeName The full name of a node type such as "enzo::grid", or of an alias such as
     * "enzo::mountain", which creates a node of the type it stands in for with the alias's
     * starting values set.
     * @param parent The scope to create the node inside. The root holds the top level nodes.
     * @param name The name to give the node. Left empty a free one is picked, and a name
     * already taken by a sibling has a number appended until it is free.
     * @param position Where the node sits in the network view.
     *
     * @return The node ID of the newly created node
     */
    NodeId createNode(
        const std::string& typeName,
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

    /// @brief Returns the frame the scene sits on.
    /// @note A fractional frame is allowed, so a sample can fall between two frames.
    floatT getFrame() const { return frame_; }

    /// @brief Moves the scene to a frame, clamped to the playback range.
    void setFrame(floatT frame);

    /// @brief Returns the first frame of the playback range.
    intT getStartFrame() const { return startFrame_; }

    /// @brief Returns the last frame of the playback range.
    intT getEndFrame() const { return endFrame_; }

    /// @brief Sets the frame the playback range starts on.
    /// @note The end frame moves up with it rather than letting the range invert.
    void setStartFrame(intT frame);

    /// @brief Sets the frame the playback range ends on.
    /// @note The start frame moves down with it rather than letting the range invert.
    void setEndFrame(intT frame);

    /// @brief Returns how many frames make up a second of playback.
    floatT getFps() const { return fps_; }

    /// @brief Sets how many frames make up a second of playback.
    /// @note A rate of zero or less is ignored, since it has no time to measure.
    void setFps(floatT fps);

    /// @brief Returns the current frame as seconds, where the start of frame 1 is zero.
    /// @return Frame 25 at 24 fps gives 1.0.
    floatT getTime() const { return (frame_ - 1) / fps_; }

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
     * @throws std::out_of_range when no scope sits at the path's parent.
     *
     * @note Only the node itself is created, not the parameter values or node links it had.
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

    /**
     * @brief Returns a copy of one of a node's outputs.
     *
     * Naming the node directly reaches geometry that no node link leads to, such as an
     * output node sitting inside a container's child scope.
     *
     * @note The node cooks first, so the geometry is never stale.
     * @return A copy the caller owns, so later cooks of the node leave it untouched.
     */
    enzo::NodePacket cookOutput(enzo::nt::NodeId nodeId, unsigned int outputIndex);

    /// @brief Returns the graph that owns the network's wiring and dependencies.
    nt::NetworkGraph& graph() { return network_.graph(); }

    /**
     * @brief Returns how many inputs a node currently takes.
     *
     * @note A single port holds one input. A multi input port holds one input
     * per node link it has.
     */
    unsigned int getInputCount(NodeId nodeId);

    /// @brief Wires one node's output into another node's input.
    /// @return The node link that was created.
    /// @note A single input port holds one node link, so whatever was on it is
    /// replaced.
    /// @note A multi input port makes room, moving the node links from the index
    /// onward up one.
    nt::NodeLink connectNodes(
        NodeId inputNodeId,
        unsigned int inputIndex,
        NodeId outputNodeId,
        unsigned int outputIndex
    );

    /// @brief Removes a node link between two nodes.
    /// @note Leaving a multi input port closes the gap, moving the node links
    /// above the index down one.
    void disconnectNodes(const nt::NodeLink& nodeLink);

    /// @brief Resolves a node reference such as "grid1" or "../grid1" to its node.
    /// @note A relative path is read from the scope holding @p fromNode.
    Node* findNode(const NetworkPath& path, NodeId fromNode = nullNode)
    {
        return network_.findNode(path, fromNode);
    }

    /// @brief Resolves a parameter reference such as "grid1.tx" to its parameter.
    /// @note A path with no node part resolves against @p fromNode.
    std::weak_ptr<prm::NodeParameter>
    findParameter(const NetworkPath& path, NodeId fromNode = nullNode)
    {
        return network_.findParameter(path, fromNode);
    }

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

    // @brief A signal emitted when a node link is created between two nodes
    boost::signals2::signal<void(nt::NodeLink)> nodeLinkCreated;

    // @brief A signal emitted when a node link is removed between two nodes
    boost::signals2::signal<void(nt::NodeLink)> nodeLinkRemoved;

    // @brief A signal emitted when the network is cleared
    boost::signals2::signal<void()> networkCleared;

    // @brief A signal emitted when a node's position changes programmatically (e.g. undo/redo)
    boost::signals2::signal<void(nt::NodeId, Vector2)> nodePositionChanged;

    // @brief A signal emitted when the scene moves to a different frame
    boost::signals2::signal<void(floatT frame)> frameChanged;

    // @brief A signal emitted when the playback range changes
    boost::signals2::signal<void(intT startFrame, intT endFrame)> frameRangeChanged;

    // @brief A signal emitted when the playback rate changes
    boost::signals2::signal<void(floatT fps)> fpsChanged;
    /** @} */

    UndoStack& undoStack() { return undoStack_; }

    /// @brief For use in unit tests, resets the state of the node.
    /// @todo Find a cleaner way to give tests a fresh manager so this
    /// doesn't pollute the public functions.
    void _reset();

  private:
    NetworkManager() {};

    // functions
    /// @brief Removes all of a node's links, each as its own undo command.
    void disconnectNode(NodeId nodeId);

    /// @brief Moves the node links at or above @p fromIndex up one input.
    void openInputGap(NodeId nodeId, unsigned int fromIndex);

    /// @brief Moves the node links above @p fromIndex down one input.
    void closeInputGap(NodeId nodeId, unsigned int fromIndex);

    /// @brief Moves a node link onto a different input of the same node.
    void moveInput(const nt::NodeLink& nodeLink, unsigned int inputIndex);

    /**
     * @brief Slot called when a node of @p NodeId is dirtied
     */
    void onNodeDirtied(nt::NodeId nodeId, bool dirtyDependents);

    /// @brief Dirties each unit's node and announces the change on parameter units.
    /// @note Takes the whole affected chain, since dirtying a unit here does not propagate.
    void dirtyUnits_(const std::vector<nt::Unit>& units);

    /// @brief Dirties every unit that reads the scene time.
    void dirtyTimeDependents_();

    /// @brief Puts the frame, the playback range, and the rate back to their defaults.
    void resetTime_();

    // variables
    // every node, its wiring, and the scopes they live in
    nt::Network network_;
    std::vector<enzo::nt::NodeId> selectedNodes_;
    // node selected for displaying in the viewport
    std::optional<NodeId> displayNode_ = std::nullopt;
    // the primary node that drives the parameter and geometry panes
    std::optional<NodeId> primaryNode_ = std::nullopt;

    // the frame the scene sits on, and the range playback runs over
    floatT frame_ = 1;
    intT startFrame_ = 1;
    intT endFrame_ = 240;
    floatT fps_ = 24;

    UndoStack undoStack_;
};

inline enzo::nt::NetworkManager& nm() { return enzo::nt::NetworkManager::getInstance(); }
} // namespace enzo::nt
