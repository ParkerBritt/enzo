#include "Engine/Operator/Context.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/Parameter.h"
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

enzo::geo::Primitive enzo::op::Context::cloneInputGeo(unsigned int inputIndex)
{
    // TODO: implement
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    auto inputConnection = selfOp.getInputConnection(inputIndex);
    if(!inputConnection.has_value())
    {
        return enzo::geo::Primitive();
    }
    const nt::GeometryConnection& connection = inputConnection.value().get();
    const nt::OpId opId = connection.getInputOpId();
    const nt::GeometryOperator& geoOp = networkManager_.getGeoOperator(opId);
    networkManager_.cookOp(opId);
    return geoOp.getOutputGeo(connection.getInputIndex());
}

bool enzo::op::Context::hasInput(unsigned int inputIndex)
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    return selfOp.getInputConnection(inputIndex).has_value();
}

// TODO: cache value
enzo::bt::floatT enzo::op::Context::evalFloatParm(const char* parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::Parameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalFloat(index);
    }
    else
    {
        throw std::runtime_error("Parameter weak ptr invalid");
    }
}

// TODO: cache value
enzo::bt::intT enzo::op::Context::evalIntParm(const char* parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::Parameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInt(index);
    }
    else
    {
        throw std::runtime_error("Parameter weak ptr invalid");
    }
}

// TODO: cache value
enzo::bt::boolT enzo::op::Context::evalBoolParm(const char* parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::Parameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInt(index);
    }
    else
    {
        throw std::runtime_error("Parameter weak ptr invalid");
    }
}

// TODO: cache value
enzo::bt::String enzo::op::Context::evalStringParm(const char* parmName, const unsigned int index) const
{
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::Parameter> parameter = selfOp.getParameter(parmName);

    if(auto sharedParm = parameter.lock())
    {
        return sharedParm->evalString(index);
    }
    else
    {
        throw std::runtime_error("Parameter weak ptr invalid");
    }
}
