#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/UndoCommand.h"

namespace enzo::nt {

class MoveNodeCommand : public UndoCommand
{
  public:
    MoveNodeCommand(NodeId nodeId, Vector2 oldPos, Vector2 newPos)
        : nodeId_(nodeId), oldPos_(oldPos), newPos_(newPos)
    {
    }

    void undo() override
    {
        nm().getNode(nodeId_).setPosition(oldPos_);
        nm().nodePositionChanged(nodeId_, oldPos_);
    }

    void redo() override
    {
        nm().getNode(nodeId_).setPosition(newPos_);
        nm().nodePositionChanged(nodeId_, newPos_);
    }

    UndoCommandType type() const override { return UndoCommandType::MoveNode; }

  private:
    NodeId nodeId_;
    Vector2 oldPos_;
    Vector2 newPos_;
};

} // namespace enzo::nt
