#pragma once
#include "Engine/Operator/GeometryConnection.h"
#include "Engine/Operator/OpInfo.h"
#include "Engine/Operator/GeometryOpDef.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include <functional>
#include <optional>
#include <memory>

namespace enzo::nt {
std::weak_ptr<GeometryConnection> connectOperators(enzo::nt::OpId inputOpId, unsigned int inputIndex, enzo::nt::OpId outputOpId, unsigned int outputIndex);

/**
* @class GeometryOperator
* @brief The unique runtime representation of a node
*/
class GeometryOperator
{
public:
    /**
    * @brief Constructs a new node
    *
    * @param opId the operator id assigned to this node. For most situations
    * this should be set by the nt::NetworkManager
    * @param opInfo The data class informing the node what its properties
    * are that set it apart from other nodes. This is what makes a grid
    * node different to a transform node.
    */
    GeometryOperator(enzo::nt::OpId opId, op::OpInfo opInfo);

    /// @brief Deleted copy constructor to avoid accidental copies. 
    GeometryOperator(const GeometryOperator&)            = delete;
    /// @brief Deleted copy assignment operator to avoid accidental copies. 
    GeometryOperator& operator=(const GeometryOperator&) = delete;

    /// @brief Computes the output geometry based on the [cookOp](@ref nt::GeometryOpDef::cookOp)
    /// definition in nt::GeometryOpDef. This is set by the @p opInfo constructor parameter
    void cookOp(op::Context context);

    /**
     * @brief Returns the current output geometry.
     *
     * Does not trigger a cook so if the geometry may be outdated if not cooked first.
     *
     * @todo Add option to force cook or cook if dirty.
     */
    geo::Primitive& getOutputGeo(unsigned int outputIndex) const;

    /** @brief Adds a GeometryConnection to one of the inputs. Replacing old connections if needed.
    *
    * Which input is decided and stored on the connection.
    *
    * Nodes can only have one connection so it will automatically remove existing connections
    * with the same index, prioritizing the new one.
    */
    void addInputConnection(std::shared_ptr<nt::GeometryConnection> connection);

    /** @brief Adds a GeometryConnection to one of the outputs. Replacing old connections if needed.
    *
    * Which output is decided and stored on the connection.
    *
    * Nodes can only have one connection so it will automatically remove existing connections
    * with the same index, prioritizing the new one.
    */
    void addOutputConnection(std::shared_ptr<nt::GeometryConnection> connection);

    /** @brief Removes an input from the node's container.
    *
    * Does not remove the connection from any other node it's connected
    * to, likely causing undefined behavior if called incorrectly.
    *
    * @todo remove in favor of the rewrite suggested in GeometryConnection
    * todo in which connections are handled by the network manager rather than individual nodes.
    */
    void removeInputConnection(unsigned int inputIndex);

    /** @brief Removes an output from the node's container.
    *
    * Does not remove the connection from any other node it's connected
    * to, likely causing undefined behavior if called incorrectly.
    *
    * @todo remove in favor of the rewrite suggested in GeometryConnection
    * todo in which connections are handled by the network manager rather than individual nodes.
    */
    void removeOutputConnection(const nt::GeometryConnection* connection);

    /** 
    * @brief Returns a vector containing weak pointers for all input connections.
    *
    * Connections returned by this function are weak pointers to indicate
    * ownership belongs to the node/network and can be modified or deleted at any time.
    */
    std::vector<std::weak_ptr<const GeometryConnection>> getInputConnections() const;

    /** 
    * @brief Returns a vector containing weak pointers for all output connections.
    *
    * Connections returned by this function are weak pointers to indicate
    * ownership belongs to the node/network and can be modified or deleted at any time.
    */
    std::vector<std::weak_ptr<const GeometryConnection>> getOutputConnections() const;

    /** 
    * @brief Returns an optional connection from a specific input index.
    *
    * @returns Nullopt if the connection doesn't exist.
    */
    std::optional<std::reference_wrapper<const GeometryConnection>> getInputConnection(size_t index) const;

    /// @brief Returns all parameters belonging to this node.
    std::vector<std::weak_ptr<prm::Parameter>> getParameters();

    /// @brief Returns a parameter with the given name belonging to this node.
    /// @returns Empty default constructed std::weak_ptr<prm::Parameter>() if no parameter of that name exists.
    std::weak_ptr<prm::Parameter> getParameter(std::string parameterName);

    /**
    * @brief NOT YET IMPLEMENTED. Returns the runtime label given to this node as a unique identifier within it's scope.
    * @todo implement
    */
    std::string getLabel(); // TODO: implement node labels

    /**
    * @brief Returns the name belonging to this type of node (eg. grid or transform). Not to be confused with the label.
    *
    * The type name decided at compile time and is shared across all nodes
    * of the given type. All grids nodes share the same type name. Labels
    * on the other hand are unique identifiers for a given runtime node (eg. myGrid1, groundGrid, wall).
    */
    std::string getTypeName();

    /** 
    * @brief Marks the outputed geometry as outdated and notifies the network
    *
    * @param dirtyDescendents Sets whether all descendents (nodes connected
    * directly or indirectly to the output of this node) are also dirtied.
    * This is usually what you want.
    */
    void dirtyNode(bool dirtyDescendents=true);

    /// @brief Returns true if the node is dirty and false if the node is clean (does not need cooking).
    bool isDirty();

    /// @brief Returns the minimum number of input connections required
    /// for the node to function. These are in order so 3 would mean the
    /// first three inputs must have a connection.
    unsigned int getMinInputs() const;
    /// @brief Returns the maximum number of input connections accepted by the node.
    unsigned int getMaxInputs() const;
    /// @brief Returns the number of available outputs the node provides.
    unsigned int getMaxOutputs() const;

    /// @brief Returns the node's position in the network graph.
    bt::Vector2f getPosition() const { return position_; }
    /// @brief Sets the node's position in the network graph.
    void setPosition(bt::Vector2f pos) { position_ = pos; }

    /// @brief A signal emitted when the node is dirtied. This will usually notify the NetworkManager
    boost::signals2::signal<void (nt::OpId opId, bool dirtyDescendents)> nodeDirtied;

private:
    void initParameters();

    // TODO: avoid duplicate connections
    std::vector<std::shared_ptr<nt::GeometryConnection>> inputConnections_;
    std::vector<std::shared_ptr<nt::GeometryConnection>> outputConnections_;
    std::vector<std::shared_ptr<prm::Parameter>> parameters_; 
    std::unique_ptr<enzo::nt::GeometryOpDef> opDef_;
    enzo::nt::OpId opId_;
    enzo::op::OpInfo opInfo_;
    bt::Vector2f position_{0.f, 0.f};
    bool dirty_ = true;
};
}
