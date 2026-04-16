#pragma once
#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Context.h"
#include "Engine/Types.h"
#include <boost/config.hpp>
#include <memory>
#include <vector>
#include "Engine/Operator/OpInfo.h"


// forward declaration
namespace enzo::nt {class NetworkManager;}

namespace enzo::nt
{
class NetworkManager;

/**
* @brief Abstract class used to create new operators.
*
* The operator definition is a base class from which new geometry operators
* are inherited from. It provides and abstracted interface, to read and
* write data about itself and the context it is being computed.
*
* The class exposes utility functions for setting outputs and reading information
* about itself like the number of inputs.
*
* The most important part of this node is the virtual cookOp member function.
* This must be overridden to implement the node's logic when being cooked.
* When a node is cooked it takes the optional input geometry from the context
* class and outputs new geometry based on the purpose of that operator.
*/
class BOOST_SYMBOL_EXPORT GeometryOpDef
{
public:
    /**
    * @brief Sets up internal state
    */
    GeometryOpDef(nt::NetworkManager* network, op::OpInfo opInfo);
    virtual ~GeometryOpDef() {};

    /**
    * @brief This function is called at runtime to create the output geometry
    *
    * @post When overriding, this function must call setOutputGeo(n) at
    * the end of a successful cook. Any outputs that are not set will output
    * an emtpy geometry object.
    */
    virtual void cookOp(op::Context context) = 0;

    /**
     * @brief Returns the current output geometry as a shared pointer.
     *
     * For use by the runtime Node
     * Does not force a cook.
     *
     * @todo move to friend class Node
     */
    std::shared_ptr<const enzo::NodePacket> getOutputPacket(unsigned outputIndex);

    /**
    * @brief Stops the cook and displays an error. Use inside the #cookOp function.
    * @todo Add visual error to GUI
    */
    void throwError(std::string error);

    /// @brief Returns the minimum number of input connections required for the node to function. Set by op::OpInfo when registering the operator.
    unsigned int getMinInputs() const;
    /// @brief Returns the maximum number of input connections accepted by the node. Set by op::OpInfo when registering the operator.
    unsigned int getMaxInputs() const;
    /// @brief Returns the number of available outputs the node provides. Set by op::OpInfo when registering the operator.
    unsigned int getMaxOutputs() const;
private:
    std::vector<std::shared_ptr<const enzo::NodePacket>> outputPackets_;

protected:
    const op::OpInfo opInfo_;
    nt::NetworkManager* network_;
    bool outputRequested(unsigned int outputIndex);
    // TODO: std::move geometry instead of copying
    void setOutputPacket(unsigned int outputIndex, enzo::NodePacket packet);
};

using opConstructor = GeometryOpDef* (*)(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo);

}
