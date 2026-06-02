#include "Engine/Operator/Context.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Types.h"
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <icecream.hpp>


enzo::op::Context::Context(enzo::nt::OpId opId, enzo::nt::NetworkManager& networkManager)
: opId_{opId}, networkManager_(networkManager)
{

}

enzo::NodePacket enzo::op::Context::cloneInputPacket(unsigned int inputIndex)
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    auto inputConnection = selfOp.getInputConnection(inputIndex).lock();
    if(!inputConnection)
    {
        return enzo::NodePacket();
    }
    const nt::OpId opId = inputConnection->getInputOpId();
    const nt::GeometryOperator& geoOp = networkManager_.getGeoOperator(opId);
    networkManager_.cookOp(opId);
    return geoOp.getOutputPacket(inputConnection->getInputIndex())->deepCopy();
}

bool enzo::op::Context::hasInput(unsigned int inputIndex)
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    return !selfOp.getInputConnection(inputIndex).expired();
}

// TODO: cache value
enzo::floatT enzo::op::Context::evalFloatParm(std::string_view parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
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
enzo::intT enzo::op::Context::evalIntParm(std::string_view parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
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
enzo::boolT enzo::op::Context::evalBoolParm(std::string_view parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
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
enzo::String enzo::op::Context::evalStringParm(std::string_view parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
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
