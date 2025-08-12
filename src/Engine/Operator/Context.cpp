#include "Engine/Operator/Context.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Types.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <icecream.hpp>


enzo::op::Context::Context(enzo::nt::OpId opId, enzo::nt::NetworkManager& networkManager)
: opId_{opId}, networkManager_(networkManager)
{

}

enzo::geo::Geometry enzo::op::Context::cloneInputGeo(unsigned int inputIndex)
{
    // TODO: implement
    enzo::nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::vector<std::weak_ptr<const nt::GeometryConnection>> inputConnections = selfOp.getInputConnections();
    if(inputConnections.size()==0)
    {
        std::cout << "no input\n";
        return enzo::geo::Geometry();
    }
    auto inputConnection = inputConnections.at(inputIndex);
    if(auto inputConnectionSP = inputConnection.lock())
    {
        const nt::GeometryOperator& geoOp = networkManager_.getGeoOperator(inputConnectionSP->getInputOpId());
        return geoOp.getOutputGeo(inputConnectionSP->getInputIndex());
    }
    else
    {
        throw std::runtime_error("Connection weak ptr doesn't exist");
    }
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
