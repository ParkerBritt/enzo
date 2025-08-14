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



void enzo::nt::GeometryOpDef::setOutputGeometry(unsigned int outputIndex, enzo::geo::Geometry geometry)
{
    if(outputIndex>getMaxOutputs())
    {
        throw std::runtime_error("Cannot set output geometry to index > maxOutputs");
    }
    outputGeometry_[outputIndex] = geometry;
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
    outputGeometry_ = std::vector<enzo::geo::Geometry>(getMaxOutputs(), enzo::geo::Geometry());
}

enzo::geo::Geometry& enzo::nt::GeometryOpDef::getOutputGeo(unsigned outputIndex)
{
    if(outputIndex>getMaxOutputs())
    {
        throw std::runtime_error("Cannot set output geometry to index > maxOutputs");
    }

    return outputGeometry_.at(outputIndex);
}

