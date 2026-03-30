#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/Types.h"

namespace enzo::nt {

class DeleteNodeCommand : public UndoCommand
{
public:
    DeleteNodeCommand(OpId opId, std::string typeName, bt::Vector2f position)
        : opId_(opId), typeName_(std::move(typeName)), position_(position) {}

    void undo() override
    {
        auto opInfo = op::OperatorTable::getOpInfo(typeName_);
        nm().restoreOperator(opId_, opInfo.value(), position_);
    }

    void redo() override
    {
        nm().removeOperator(opId_);
    }

private:
    OpId opId_;
    std::string typeName_;
    bt::Vector2f position_;
};

}
