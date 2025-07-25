#include <oneapi/tbb/parallel_for.h>
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

const enzo::geo::Geometry& enzo::nt::GeometryOpDef::getInputGeoView(unsigned int inputIndex)
{
    // TODO: implement
    return enzo::geo::Geometry();

}


void enzo::nt::GeometryOpDef::setOutputGeometry(unsigned int outputIndex, enzo::geo::Geometry geometry)
{
    if(outputIndex>maxOutputs_)
    {
        throw std::runtime_error("Cannot set output geometry to index > maxOutputs");
    }
    outputGeometry_[outputIndex] = geometry;
}

enzo::nt::GeometryOpDef::GeometryOpDef(enzo::nt::OpId opId)
: opId_{opId}
{
    minInputs_=1;
    maxInputs_=4;
    maxOutputs_=4;
    outputGeometry_ = std::vector<enzo::geo::Geometry>(4, enzo::geo::Geometry());
}

enzo::geo::Geometry& enzo::nt::GeometryOpDef::getOutputGeo(unsigned outputIndex)
{
    if(outputIndex>maxOutputs_)
    {
        throw std::runtime_error("Cannot set output geometry to index > maxOutputs");
    }

    return outputGeometry_.at(outputIndex);
}

