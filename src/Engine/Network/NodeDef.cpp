#include "Engine/Network/NodeDef.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NetworkManager.h"
#include <iostream>
#include <stdexcept>
#include <tbb/parallel_for.h>

namespace enzo {

bool nt::NodeDef::outputRequested(unsigned int outputIndex)
{
    // TODO: implement
    return true;
}

void nt::NodeDef::setOutputPacket(unsigned int outputIndex, NodePacket packet)
{
    if (outputIndex > getMaxOutputs())
    {
        throw std::runtime_error("Cannot set output packet to index > maxOutputs");
    }

    // Auto-defragment every primitive so downstream consumers see contiguous offsets.
    for (auto& prim : packet.getPrimitives())
    {
        if (prim) prim->defragment();
    }

    outputPackets_[outputIndex] = std::make_shared<const NodePacket>(std::move(packet));
}

void nt::NodeDef::throwError(std::string error)
{
    std::cerr << "NODE EXCEPTION: " << error << "\n";
}

void nt::NodeDef::throwWarning(std::string warning)
{
    std::cerr << "NODE WARNING: " << warning << "\n";
}

unsigned int nt::NodeDef::getMinInputs() const { return nodeType_.minInputs; }

unsigned int nt::NodeDef::getMaxInputs() const { return nodeType_.maxInputs; }

unsigned int nt::NodeDef::getMaxOutputs() const { return nodeType_.maxOutputs; }

nt::NodeDef::NodeDef(nt::NetworkManager* network, nt::NodeType nodeType)
    : nodeType_{nodeType}, network_{network}
{
    // Initialize each slot with an empty packet so consumers never see null.
    outputPackets_.resize(getMaxOutputs());
    for (auto& slot : outputPackets_)
    {
        slot = std::make_shared<const NodePacket>();
    }
}

std::shared_ptr<const NodePacket> nt::NodeDef::getOutputPacket(unsigned outputIndex)
{
    if (outputIndex > getMaxOutputs())
    {
        throw std::runtime_error("Cannot get output packet at index > maxOutputs");
    }

    return outputPackets_.at(outputIndex);
}

} // namespace enzo
