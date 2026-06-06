#include "Engine/Network/Context.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/NodeParameter.h"
#include <exception>
#include <icecream.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace enzo {

op::Context::Context(nt::OpId opId, nt::NetworkManager& networkManager)
    : opId_{opId}, networkManager_(networkManager)
{
}

NodePacket op::Context::cloneInputPacket(unsigned int inputIndex)
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    auto inputConnection = selfOp.getInputConnection(inputIndex).lock();
    if (!inputConnection)
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
floatT op::Context::evalParmFloat(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalFloat(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

// TODO: cache value
intT op::Context::evalParmInt(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInt(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

// TODO: cache value
boolT op::Context::evalParmBool(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInt(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

// TODO: cache value
String op::Context::evalParmString(std::string_view parmName, const unsigned int index) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalString(index);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

std::vector<floatT> op::Context::evalParmFloats(std::string_view parmName) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalFloats();
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

Vector2 op::Context::evalParmVector2(std::string_view parmName) const
{
    std::vector<floatT> components = evalParmFloats(parmName);
    Vector2 result = Vector2::Zero();
    for (size_t componentIndex = 0; componentIndex < components.size() && componentIndex < 2;
         ++componentIndex)
    {
        result[componentIndex] = components[componentIndex];
    }
    return result;
}

Vector3 op::Context::evalParmVector3(std::string_view parmName) const
{
    std::vector<floatT> components = evalParmFloats(parmName);
    Vector3 result = Vector3::Zero();
    for (size_t componentIndex = 0; componentIndex < components.size() && componentIndex < 3;
         ++componentIndex)
    {
        result[componentIndex] = components[componentIndex];
    }
    return result;
}

std::vector<intT> op::Context::evalParmInts(std::string_view parmName) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalInts();
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

std::vector<String> op::Context::evalParmStrings(std::string_view parmName) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return sharedParm->evalStrings();
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

} // namespace enzo
