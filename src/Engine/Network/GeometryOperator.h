#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Network/GeometryConnection.h"
#include "Engine/Network/GeometryOpDef.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Network/OpInfo.h"
#include "Engine/Parameter/NodeParameter.h"
#include <functional>
#include <memory>
#include <optional>

namespace enzo::nt {
std::weak_ptr<GeometryConnection> connectOperators(
    enzo::nt::OpId inputOpId,
    unsigned int inputIndex,
    enzo::nt::OpId outputOpId,
    unsigned int outputIndex
);

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
    virtual ~GeometryOperator() = default;
    /// @brief Deleted copy constructor to avoid accidental copies.
    GeometryOperator(const GeometryOperator&) = delete;
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
    std::shared_ptr<const enzo::NodePacket> getOutputPacket(unsigned int outputIndex) const;

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
    std::vector<std::weak_ptr<GeometryConnection>> getInputConnections() const;

    /**
     * @brief Returns a vector containing weak pointers for all output connections.
     *
     * Connections returned by this function are weak pointers to indicate
     * ownership belongs to the node/network and can be modified or deleted at any time.
     */
    std::vector<std::weak_ptr<GeometryConnection>> getOutputConnections() const;

    /**
     * @brief Returns an optional connection from a specific input index.
     *
     * @returns Nullopt if the connection doesn't exist.
     */
    std::weak_ptr<GeometryConnection> getInputConnection(size_t index) const;

    /// @brief Returns all parameters belonging to this node.
    std::vector<std::weak_ptr<prm::NodeParameter>> getParameters();

    /// @brief Returns a parameter with the given name belonging to this node.
    /// @returns Empty default constructed std::weak_ptr<prm::Parameter>() if no parameter of that
    /// name exists.
    std::weak_ptr<prm::NodeParameter> getParameter(std::string_view parameterName);

    /// @brief Whether the named parameter's disable condition leaves it enabled.
    /// @return True when the condition is empty, unparsable, or not met.
    bool isParameterEnabled(std::string_view parmName);

    /// @brief Whether the named parameter's hide condition currently hides it.
    /// @return True only when the condition is present and met.
    bool isParameterHidden(std::string_view parmName);

    /// @brief Returns the template tree declared by this node's type.
    /// @returns Top level templates. Group templates contain child templates recursively.
    const std::vector<prm::Template>& getTemplates() const;

    /**
     * @brief Returns the runtime name uniquely identifying this node within its scope (eg.
     * "my_node_05")
     *
     * Unlike the type name this is per node and is intended to be user assignable.
     * @note Uniqueness checked, user assignable labels are not yet implemented. The name is
     * currently synthesized from the type name and op id.
     * @todo implement user assignable, uniqueness checked node names
     */
    std::string getName();

    /**
     * @brief Returns the static type definition this node was created from.
     *
     * The type carries data shared by every node of the same type such as its
     * name and label. Use getType().getName() for the internal type name (eg.
     * "copy_to_points") and getType().getLabel() for the display label (eg. "Copy To Points").
     */
    const op::OpInfo& getType() const;

    /**
     * @brief Marks the outputed geometry as outdated and notifies the network
     *
     * @param dirtyDescendents Sets whether all descendents (nodes connected
     * directly or indirectly to the output of this node) are also dirtied.
     * This is usually what you want.
     */
    void dirtyNode(bool dirtyDescendents = true);

    /// @brief Returns true if the node is dirty and false if the node is clean (does not need
    /// cooking).
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
    Vector2 getPosition() const { return position_; }
    /// @brief Sets the node's position in the network graph.
    void setPosition(Vector2 pos) { position_ = pos; }

    /// @brief A signal emitted when the node is dirtied. This will usually notify the
    /// NetworkManager
    boost::signals2::signal<void(nt::OpId opId, bool dirtyDescendents)> nodeDirtied;

    /// @brief A signal emitted when one parameter's value changes, carrying its name.
    boost::signals2::signal<void(const std::string& parmName)> parameterChanged;

  private:
    void initParameters();

    /// @brief Whether a disable or hide comparison currently evaluates true.
    /// @return True only when the comparison is present, parses, and is met.
    bool isComparisonTrue(const std::string& conditionText);

    /// @brief Notifies observers a parameter changed and dirties the node.
    void onParameterChanged(const std::string& parmName);

    // TODO: avoid duplicate connections
    std::vector<std::shared_ptr<nt::GeometryConnection>> inputConnections_;
    std::vector<std::shared_ptr<nt::GeometryConnection>> outputConnections_;
    std::vector<std::shared_ptr<prm::NodeParameter>> parameters_;
    std::unique_ptr<enzo::nt::GeometryOpDef> opDef_;
    enzo::nt::OpId opId_;
    enzo::op::OpInfo opInfo_;
    Vector2 position_{0.f, 0.f};
    bool dirty_ = true;
};
} // namespace enzo::nt
