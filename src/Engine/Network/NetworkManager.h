#pragma once
#include "Engine/Network/UpdateLock.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/UndoRedo/UndoStack.h"
#include "Engine/Types.h"
#include <memory>
#include <unordered_map>
#include <QObject>

namespace enzo::nt {
/**
* @brief The central coordinator of the engine's node system.
*
* The network manager is the central coordinator of the engine’s node system.
* It manages the lifecycle of operators, including their creation, storage,
* and validation, while also tracking dependencies between them. Acting
* as a singleton, it ensures that all parts of the engine work with a single
* consistent view of the network, providing global access. Beyond just storing
* operators, it also controls cooking and traversing dependency graphs,
* ensuring that updates flow correctly through the network when nodes change.
*
* @todo remove Qobject inheritance, this is no longer needed since switching to boost signals.
*
*/
class NetworkManager
: public QObject
{
    // TODO: make not Q_OBJECT
    Q_OBJECT
public:

    /// @brief Iterable range over operators, yields {OpId, GeometryOperator&} pairs.
    class OperatorRange {
        using Map = std::unordered_map<OpId, std::unique_ptr<GeometryOperator>>;
        Map& map_;
    public:
        class Iterator {
            Map::iterator it_;
        public:
            using value_type = std::pair<const OpId, GeometryOperator&>;
            using reference = value_type;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            Iterator(Map::iterator it) : it_(it) {}
            reference operator*() const { return {it_->first, *it_->second}; }
            Iterator& operator++() { ++it_; return *this; }
            Iterator operator++(int) { Iterator tmp = *this; ++it_; return tmp; }
            bool operator==(const Iterator& other) const { return it_ == other.it_; }
            bool operator!=(const Iterator& other) const { return it_ != other.it_; }
        };

        OperatorRange(Map& map) : map_(map) {}
        Iterator begin() { return Iterator(map_.begin()); }
        Iterator end() { return Iterator(map_.end()); }
        std::size_t size() const { return map_.size(); }
    };

    /// @brief Returns an iterable range over all operators in the network.
    OperatorRange operators() { return OperatorRange(gopStore_); }

    /// @brief Deleted the copy constructor for singleton.
    NetworkManager(const NetworkManager& obj) = delete;

    /// @brief Returns a reference to the singleton instance.
    static NetworkManager& getInstance();

    /**
    * @brief Creates a new node in the network
    *
    * @param OpInfo Data designating the properties of the node.
    *
    * @returns The operator ID of the newly created node
    *
    * @todo Should probably only have to pass type, now entire opInfo. Fix soon!!!
    */
    OpId createOperator(op::OpInfo opInfo, bt::Vector2f position = {0.f, 0.f});

    /** @brief Returns the operator ID for the node with its display flag set.
    * There can only be only be one operator displayed at a time.
    * Return value is nullopt if no node is set to display
    */
    std::optional<OpId> getDisplayOp();

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
    * @param opId Operator ID of the node to check the validity of.
    */
    bool isValidOp(nt::OpId opId);

    /**
    * @brief Returns a reference to the GeometryOperator with the given OpId
    */
    GeometryOperator& getGeoOperator(nt::OpId opId);

    /**
    * @brief Sets given OpId to be displayed, releasing previous display Node
    */
    void setDisplayOp(OpId opId);

    /**
    * @brief Clears the display flag so no node is displayed
    */
    void clearDisplayFlag();

    /**
    * @brief Set the selection state for the given node.
    *
    * @param opId The node to set the state on.
    * @param selected The selection state, true selects the node, false unselects it.
    * @param add By default all other nodes are unselected, this parameter
    * allows adding a selected node without deslecting any others.
    */
    void setSelectedNode(OpId opId, bool selected, bool add=false);

    /**
    * @brief Returns the OpIds for all selected nodes.
    */
    const std::vector<enzo::nt::OpId>& getSelectedNodes();

    /**
    * @brief Replaces the entire selection with the given set of nodes.
    */
    void setSelectedNodes(std::vector<enzo::nt::OpId> opIds);

    /**
     * @brief Moves a node to a new position, pushing an undo command.
     * @param opId The operator to move.
     * @param newPos The new position.
     *
     * @todo remove skipUndo argument in favour of a global undo RAII lock
     */
    void moveNode(OpId opId, bt::Vector2f newPos, bool skipUndo=false);

    /**
     * @brief Deletes a node, pushing an undo command.
     * @param opId The operator to delete.
     */
    void deleteNode(OpId opId);

    /**
     * @brief Removes an operator and all its connections from the network.
     * @param opId The operator to remove.
     */
    void removeOperator(OpId opId);

    /**
     * @brief Restores a previously removed operator with a specific OpId.
     *
     * @note This does not restore the state the node was in, only creates a new node with the given id.
     * @todo maybe replace with createNodeWithId
     *
     * @param opId The operator ID to restore.
     * @param opInfo The operator info.
     * @param position The position to restore the operator at.
     */
    void restoreOperator(OpId opId, op::OpInfo opInfo);

    /**
     * @brief Clears all operators and resets the network to its initial state.
     */
    void clear();

    /**
     * @brief Cooks the given operator
     * @param opId operator ID to cook
     */
    void cookOp(enzo::nt::OpId opId);

    /** @name Signals
    * @{
    */
    // @brief A signal emitted when the display node is changed
    boost::signals2::signal<void (std::optional<nt::OpId>)> displayNodeChanged;

    // @brief A signal emitted when the geometry to be displayed is changed
    // This is different to #displayNodeChanged because the state of geometry
    // in a node can change based on parameters or other factors.
    boost::signals2::signal<void (enzo::NodePacket& packet)> displayGeoChanged;

    // @brief A signal emitted when the selected node's geometry is changed
    boost::signals2::signal<void (enzo::NodePacket& packet)> selectedGeoChanged;

    // @brief A signal emitted when the selection of nodes changes
    boost::signals2::signal<void (std::vector<nt::OpId> selectedNodeIds)> selectedNodesChanged;

    // @brief A signal emitted when a new operator is created in the network
    boost::signals2::signal<void (nt::OpId)> operatorCreated;

    // @brief A signal emitted when an operator is about to be removed from the network
    boost::signals2::signal<void (nt::OpId)> operatorRemoved;

    // @brief A signal emitted when a connection is created between two operators
    boost::signals2::signal<void (std::weak_ptr<nt::GeometryConnection>)> connectionCreated;

    // @brief A signal emitted when the network is cleared
    boost::signals2::signal<void ()> networkCleared;

    // @brief A signal emitted when a node's position changes programmatically (e.g. undo/redo)
    boost::signals2::signal<void (nt::OpId, bt::Vector2f)> nodePositionChanged;
    /** @} */

    UndoStack& undoStack() { return undoStack_; }

    #ifdef UNIT_TEST
    /// @brief For use in unit tests, resets the state of the operator.
        void _reset();
    #endif

private:
    NetworkManager() {};

    // functions
    std::vector<enzo::nt::OpId> getDependencyGraph(enzo::nt::OpId opId);
    std::vector<enzo::nt::OpId> getDependentsGraph(enzo::nt::OpId opId);

    /**
     * @brief Slot called when a node of @p OpId is dirtied
     */
    void onNodeDirtied(nt::OpId opId, bool dirtyDependents);

    // variables
    // store for geometry operators
    std::vector<enzo::nt::OpId> selectedNodes_;
    std::unordered_map<enzo::nt::OpId, std::unique_ptr<enzo::nt::GeometryOperator>> gopStore_;
    // the highest operator id currently stored
    enzo::nt::OpId maxOpId_=0;
    // operator selected for displaying in the viewport
    std::optional<OpId> displayOp_=std::nullopt;

    UndoStack undoStack_;

};

inline enzo::nt::NetworkManager& nm() {
    return enzo::nt::NetworkManager::getInstance();
}
}
