#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <optional>

namespace enzo::nt {

class ChangeDisplayFlagCommand : public UndoCommand
{
  public:
    ChangeDisplayFlagCommand(std::optional<NodeId> prev, NodeId next) : prev_(prev), next_(next) {}

    void undo() override
    {
        if (prev_.has_value())
        {
            if (!nm().isValidNode(*prev_)) return;
            nm().setDisplayNode(*prev_);
        }
        else
        {
            nm().clearDisplayFlag();
        }
    }

    void redo() override
    {
        if (!nm().isValidNode(next_)) return;
        nm().setDisplayNode(next_);
    }

    UndoCommandType type() const override { return UndoCommandType::ChangeDisplayFlag; }

  private:
    std::optional<NodeId> prev_;
    NodeId next_;
};

} // namespace enzo::nt
