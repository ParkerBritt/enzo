#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/Types.h"
#include <vector>
#include <string>
#include <icecream.hpp>

namespace enzo::nt {

class CreateNodeCommand : public UndoCommand
{
public:
    CreateNodeCommand(OpId opId) : opId_(opId)
    {
    }

    void undo() override
    {
        GeometryOperator& op = nm().getGeoOperator(opId_);
        typeName_ =  op.getTypeName();
        position_ = op.getPosition();

        nm().removeOperator(opId_);
    }

    void redo() override
    {
        // Restore operator
        auto opInfo = op::OperatorTable::getOpInfo(typeName_);
        nm().restoreOperator(opId_, opInfo.value());

        // Restore position
        GeometryOperator& op = nm().getGeoOperator(opId_);
        nm().moveNode(opId_, position_, true);
    }

    UndoCommandType type() const override { return UndoCommandType::CreateNode; }

private:
    OpId opId_;
    std::string typeName_;
    bt::Vector2f position_;
};

}
