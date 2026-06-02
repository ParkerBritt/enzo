#include <tbb/parallel_for.h>
#include "Engine/Operator/GeometryOpDef.h"
#include <stdexcept>
#include <iostream>
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Types.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Network/NetworkManager.h"

namespace enzo {

bool nt::GeometryOpDef::outputRequested(unsigned int outputIndex)
{
    // TODO: implement
    return true;
}



void nt::GeometryOpDef::setOutputPacket(unsigned int outputIndex, NodePacket packet)
{
    if(outputIndex>getMaxOutputs())
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

void nt::GeometryOpDef::throwError(std::string error)
{
    std::cerr << "NODE EXCEPTION: " << error << "\n";

}

void nt::GeometryOpDef::throwWarning(std::string warning)
{
    std::cerr << "NODE WARNING: " << warning << "\n";

}

unsigned int nt::GeometryOpDef::getMinInputs() const
{
    return opInfo_.minInputs;

}

unsigned int nt::GeometryOpDef::getMaxInputs() const
{
    return opInfo_.maxInputs;
}

unsigned int nt::GeometryOpDef::getMaxOutputs() const
{
    return opInfo_.maxOutputs;
}


nt::GeometryOpDef::GeometryOpDef(nt::NetworkManager* network, op::OpInfo opInfo)
: opInfo_{opInfo}, network_{network}
{
    // Initialize each slot with an empty packet so consumers never see null.
    outputPackets_.resize(getMaxOutputs());
    for(auto& slot : outputPackets_)
    {
        slot = std::make_shared<const NodePacket>();
    }
}

std::shared_ptr<const NodePacket> nt::GeometryOpDef::getOutputPacket(unsigned outputIndex)
{
    if(outputIndex>getMaxOutputs())
    {
        throw std::runtime_error("Cannot get output packet at index > maxOutputs");
    }

    return outputPackets_.at(outputIndex);
}

} // namespace enzo
