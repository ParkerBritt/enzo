#include <tbb/parallel_for.h>
#include "Engine/Operator/GeometryOpDef.h"
#include <stdexcept>
#include <iostream>
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Types.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Network/NetworkManager.h"

bool enzo::nt::GeometryOpDef::outputRequested(unsigned int outputIndex)
{
    // TODO: implement
    return true;
}



void enzo::nt::GeometryOpDef::setOutputPacket(unsigned int outputIndex, enzo::NodePacket packet)
{
    if(outputIndex>getMaxOutputs())
    {
        throw std::runtime_error("Cannot set output packet to index > maxOutputs");
    }
    outputPackets_[outputIndex] = std::make_shared<const enzo::NodePacket>(std::move(packet));
}

void enzo::nt::GeometryOpDef::throwError(std::string error)
{
    std::cerr << "NODE EXCEPTION: " << error << "\n";

}


unsigned int enzo::nt::GeometryOpDef::getMinInputs() const
{
    return opInfo_.minInputs;

}

unsigned int enzo::nt::GeometryOpDef::getMaxInputs() const
{
    return opInfo_.maxInputs;
}

unsigned int enzo::nt::GeometryOpDef::getMaxOutputs() const
{
    return opInfo_.maxOutputs;
}


enzo::nt::GeometryOpDef::GeometryOpDef(nt::NetworkManager* network, op::OpInfo opInfo)
: opInfo_{opInfo}, network_{network}
{
    // Initialize each slot with an empty packet so consumers never see null.
    outputPackets_.resize(getMaxOutputs());
    for(auto& slot : outputPackets_)
    {
        slot = std::make_shared<const enzo::NodePacket>();
    }
}

std::shared_ptr<const enzo::NodePacket> enzo::nt::GeometryOpDef::getOutputPacket(unsigned outputIndex)
{
    if(outputIndex>getMaxOutputs())
    {
        throw std::runtime_error("Cannot get output packet at index > maxOutputs");
    }

    return outputPackets_.at(outputIndex);
}

