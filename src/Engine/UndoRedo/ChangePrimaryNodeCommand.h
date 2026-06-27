#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <optional>

namespace enzo::nt {

class ChangePrimaryNodeCommand : public UndoCommand
{
  public:
    ChangePrimaryNodeCommand(std::optional<OpId> prev, OpId next) : prev_(prev), next_(next) {}

    void undo() override
    {
        if (prev_.has_value())
        {
            if (!nm().isValidOp(*prev_)) return;
            nm().setPrimaryNode(*prev_);
        }
        else
        {
            nm().clearPrimaryNode();
        }
    }

    void redo() override
    {
        if (!nm().isValidOp(next_)) return;
        nm().setPrimaryNode(next_);
    }

    UndoCommandType type() const override { return UndoCommandType::ChangePrimaryNode; }

  private:
    std::optional<OpId> prev_;
    OpId next_;
};

} // namespace enzo::nt
