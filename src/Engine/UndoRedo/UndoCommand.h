#pragma once

#include "Engine/UndoRedo/UndoCommands.h"

namespace enzo::nt {

class UndoCommand
{
  public:
    virtual ~UndoCommand() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual UndoCommandType type() const = 0;
};

} // namespace enzo::nt
