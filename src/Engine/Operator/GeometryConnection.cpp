#include "Engine/Operator/GeometryConnection.h"
#include "Engine/Network/NetworkManager.h"
#include <icecream.hpp>

enzo::nt::GeometryConnection::GeometryConnection(enzo::nt::OpId inputOpId, unsigned int inputIndex, enzo::nt::OpId outputOpId, unsigned int outputIndex)
:inputOperatorId_{inputOpId}, inputIndex_{inputIndex}, outputOperatorId_{outputOpId}, outputIndex_{outputIndex}
{    
}

enzo::nt::OpId enzo::nt::GeometryConnection::getInputOpId() const {return inputOperatorId_; }
enzo::nt::OpId enzo::nt::GeometryConnection::getOutputOpId() const {return outputOperatorId_; }
unsigned int enzo::nt::GeometryConnection::getInputIndex() const {return inputIndex_; }
unsigned int enzo::nt::GeometryConnection::getOutputIndex() const {return outputIndex_; }

void enzo::nt::GeometryConnection::remove()
{
    NetworkManager& nm = nt::nm();
    std::cout << "removing connection " << this;
    nm.getGeoOperator(inputOperatorId_).removeOutputConnection(this);
    IC();
    nm.getGeoOperator(outputOperatorId_).removeInputConnection(outputIndex_);
    IC();
    removed();
    IC();
}



