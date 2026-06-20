#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <icecream.hpp>
#include <string>
#include <vector>

namespace enzo::nt {

class CreateNodeCommand : public UndoCommand
{
  public:
    CreateNodeCommand(OpId opId) : opId_(opId) {}

    void undo() override
    {
        GeometryOperator& op = nm().getGeoOperator(opId_);
        typeName_ = op.getType().getName();
        position_ = op.getPosition();

        nm().removeOperator(opId_, false);
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
    Vector2 position_;
};

} // namespace enzo::nt
