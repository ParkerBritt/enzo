#pragma once
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Operator/Geometry.h"
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
    Q_OBJECT
public:
    /// @brief Deleted the copy constructor for singleton.
    NetworkManager(const NetworkManager& obj) = delete;

    /// @brief Returns a reference to the singleton instance.
    static NetworkManager& getInstance();

    /**
    * @brief Creates a new node in the network
    *
    * @param OpInfo Data designating the properties of the node.
    *
    * @returns The operator ID of the newly created onode
    */
    OpId addOperator(op::OpInfo opInfo);

    /** @brief Returns the operator ID for the node with its display flag set.
    * There can only be only be one operator displayed at a time.
    * Return value is nullop if no node is set to display
    */
    std::optional<OpId> getDisplayOp();


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
    * @brief Sets given OpId to be dislayed, releasing previous display Node
    */
    void setDisplayOp(OpId opId);

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

    /** @name Signals
    * @{
    */
    // @brief A signal emitted when the display node is changed
    boost::signals2::signal<void (nt::OpId)> displayNodeChanged;

    // @brief A signal emitted when the geometry to be displayed is changed
    // This is different to #displayNodeChanged because the state of geometry
    // in a node can change based on parameters or other factors.
    boost::signals2::signal<void (enzo::geo::Geometry& geometry)> displayGeoChanged;

    // @brief A signal emitted when the selection of nodes changes
    boost::signals2::signal<void (std::vector<nt::OpId> selectedNodeIds)> selectedNodesChanged;
    /** @} */

    #ifdef UNIT_TEST
    /// @brief For use in unit tests, resets the state of the operator.
        void _reset();
    #endif

private:
    NetworkManager() {};

    // functions
    void cookOp(enzo::nt::OpId opId);
    std::vector<enzo::nt::OpId> getDependencyGraph(enzo::nt::OpId opId);
    std::vector<enzo::nt::OpId> getDependentsGraph(enzo::nt::OpId opId);

    // variables
    // store for geometry operators
    std::vector<enzo::nt::OpId> selectedNodes_;
    std::unordered_map<enzo::nt::OpId, std::unique_ptr<enzo::nt::GeometryOperator>> gopStore_;
    // the highest operator id currently stored
    enzo::nt::OpId maxOpId_=0;
    // operator selected for displaying in the viewport
    std::optional<OpId> displayOp_=std::nullopt;


};

inline enzo::nt::NetworkManager& nm() {
    return enzo::nt::NetworkManager::getInstance();
}
}
