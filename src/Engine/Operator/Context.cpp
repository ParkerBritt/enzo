#include "Engine/Operator/Context.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Core/Types.h"
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <icecream.hpp>

namespace enzo {


op::Context::Context(nt::OpId opId, nt::NetworkManager& networkManager)
: opId_{opId}, networkManager_(networkManager)
{

}

NodePacket op::Context::cloneInputPacket(unsigned int inputIndex)
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    auto inputConnection = selfOp.getInputConnection(inputIndex).lock();
    if(!inputConnection)
    {
        return NodePacket();
    }
    const nt::OpId opId = inputConnection->getInputOpId();
    const nt::GeometryOperator& geoOp = networkManager_.getGeoOperator(opId);
    networkManager_.cookOp(opId);
    return geoOp.getOutputPacket(inputConnection->getInputIndex())->deepCopy();
}

bool op::Context::hasInput(unsigned int inputIndex)
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    return !selfOp.getInputConnection(inputIndex).expired();
}

// TODO: cache value
floatT op::Context::evalFloatParm(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalFloat(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

// TODO: cache value
intT op::Context::evalIntParm(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInt(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

// TODO: cache value
boolT op::Context::evalBoolParm(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInt(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

// TODO: cache value
String op::Context::evalStringParm(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalString(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

} // namespace enzo
