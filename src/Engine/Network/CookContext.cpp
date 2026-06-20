#include "Engine/Network/CookContext.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Ramp.h"
#include <memory>
#include <stdexcept>

namespace enzo {

op::CookContext::CookContext(nt::OpId opId, nt::NetworkManager& networkManager)
    : opId_{opId}, networkManager_(networkManager)
{
}

NodePacket op::CookContext::cloneInputPacket(unsigned int inputIndex)
{
    auto inputConnection = networkManager_.graph().getInputConnection(opId_, inputIndex);
    if (!inputConnection)
    {
        return NodePacket();
    }
    const nt::OpId sourceOpId = inputConnection->sourceOp;
    const nt::GeometryOperator& sourceOp = networkManager_.getGeoOperator(sourceOpId);
    networkManager_.cookOp(sourceOpId);
    return sourceOp.getOutputPacket(inputConnection->sourceOutput)->deepCopy();
}

bool op::CookContext::hasInput(unsigned int inputIndex)
{
    return networkManager_.graph().getInputConnection(opId_, inputIndex).has_value();
}

// TODO: cache value
floatT op::CookContext::evalParmFloat(std::string_view parmName, const unsigned int index) const
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
intT op::CookContext::evalParmInt(std::string_view parmName, const unsigned int index) const
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
boolT op::CookContext::evalParmBool(std::string_view parmName, const unsigned int index) const
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
String op::CookContext::evalParmString(std::string_view parmName, const unsigned int index) const
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

prm::Ramp op::CookContext::evalParmRamp(std::string_view parmName) const
{
    nt::GeometryOperator& selfOp = networkManager_.getGeoOperator(opId_);
    std::weak_ptr<prm::NodeParameter> parameter = selfOp.getParameter(parmName);

    if (auto sharedParm = parameter.lock())
    {
        return prm::Ramp(*sharedParm);
    }
    else
    {
        throw std::runtime_error(parmName + " parameter does not exist.");
    }
}

std::vector<floatT> op::CookContext::evalParmFloats(std::string_view parmName) const
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

Vector2 op::CookContext::evalParmVector2(std::string_view parmName) const
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

Vector3 op::CookContext::evalParmVector3(std::string_view parmName) const
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

std::vector<intT> op::CookContext::evalParmInts(std::string_view parmName) const
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

std::vector<String> op::CookContext::evalParmStrings(std::string_view parmName) const
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
