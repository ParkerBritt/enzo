#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/Node.h"
#include "Engine/Parameter/Ramp.h"
#include <iostream>

namespace enzo::nt {

void NodeImpl::throwError(std::string error) { std::cerr << "NODE EXCEPTION: " << error << "\n"; }

void NodeImpl::throwWarning(std::string warning)
{
    std::cerr << "NODE WARNING: " << warning << "\n";
}

bool NodeImpl::outputRequested(unsigned int outputIndex)
{
    return node_.outputRequested(outputIndex);
}

void NodeImpl::setOutputPacket(unsigned int outputIndex, NodePacket packet)
{
    node_.setOutputPacket(outputIndex, std::move(packet));
}

NodePacket NodeImpl::cloneInputPacket(unsigned int inputIndex)
{
    return context_.cloneInputPacket(inputIndex);
}

bool NodeImpl::hasInput(unsigned int inputIndex) { return context_.hasInput(inputIndex); }

floatT NodeImpl::evalParmFloat(std::string_view parmName, const unsigned int index) const
{
    return context_.evalParmFloat(parmName, index);
}

intT NodeImpl::evalParmInt(std::string_view parmName, const unsigned int index) const
{
    return context_.evalParmInt(parmName, index);
}

boolT NodeImpl::evalParmBool(std::string_view parmName, const unsigned int index) const
{
    return context_.evalParmBool(parmName, index);
}

String NodeImpl::evalParmString(std::string_view parmName, const unsigned int index) const
{
    return context_.evalParmString(parmName, index);
}

std::vector<floatT> NodeImpl::evalParmFloats(std::string_view parmName) const
{
    return context_.evalParmFloats(parmName);
}

std::vector<intT> NodeImpl::evalParmInts(std::string_view parmName) const
{
    return context_.evalParmInts(parmName);
}

std::vector<String> NodeImpl::evalParmStrings(std::string_view parmName) const
{
    return context_.evalParmStrings(parmName);
}

Vector2 NodeImpl::evalParmVector2(std::string_view parmName) const
{
    return context_.evalParmVector2(parmName);
}

Vector3 NodeImpl::evalParmVector3(std::string_view parmName) const
{
    return context_.evalParmVector3(parmName);
}

prm::Ramp NodeImpl::evalParmRamp(std::string_view parmName) const
{
    return context_.evalParmRamp(parmName);
}

} // namespace enzo::nt
