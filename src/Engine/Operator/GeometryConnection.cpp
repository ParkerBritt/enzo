#include "Engine/Operator/GeometryConnection.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/ChangeConnectionCommand.h"
#include <icecream.hpp>

namespace enzo {

nt::GeometryConnection::GeometryConnection(nt::OpId inputOpId, unsigned int inputIndex, nt::OpId outputOpId, unsigned int outputIndex)
:inputOperatorId_{inputOpId}, inputIndex_{inputIndex}, outputOperatorId_{outputOpId}, outputIndex_{outputIndex}
{    
}

nt::OpId nt::GeometryConnection::getInputOpId() const {return inputOperatorId_; }
nt::OpId nt::GeometryConnection::getOutputOpId() const {return outputOperatorId_; }
unsigned int nt::GeometryConnection::getInputIndex() const {return inputIndex_; }
unsigned int nt::GeometryConnection::getOutputIndex() const {return outputIndex_; }

void nt::GeometryConnection::remove()
{
    NetworkManager& nm = nt::nm();
    std::cout << "removing connection " << this;

    auto cmd = std::make_unique<ChangeConnectionCommand>(
        inputOperatorId_, inputIndex_, outputOperatorId_, outputIndex_,
        ChangeConnectionCommand::Action::Disconnect);
    nm.undoStack().push(std::move(cmd));

    nm.getGeoOperator(inputOperatorId_).removeOutputConnection(this);
    IC();
    nm.getGeoOperator(outputOperatorId_).removeInputConnection(outputIndex_);
    IC();
    removed();
    IC();
}

} // namespace enzo
