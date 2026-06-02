#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"

namespace enzo::nt {

class MoveNodeCommand : public UndoCommand
{
public:
    MoveNodeCommand(OpId opId, Vector2f oldPos, Vector2f newPos)
        : opId_(opId), oldPos_(oldPos), newPos_(newPos) {}

    void undo() override
    {
        nm().getGeoOperator(opId_).setPosition(oldPos_);
        nm().nodePositionChanged(opId_, oldPos_);
    }

    void redo() override
    {
        nm().getGeoOperator(opId_).setPosition(newPos_);
        nm().nodePositionChanged(opId_, newPos_);
    }

    UndoCommandType type() const override { return UndoCommandType::MoveNode; }

private:
    OpId opId_;
    Vector2f oldPos_;
    Vector2f newPos_;
};

}
