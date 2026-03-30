#pragma once

namespace enzo::nt {

class UndoCommand
{
public:
    virtual ~UndoCommand() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
};

}
