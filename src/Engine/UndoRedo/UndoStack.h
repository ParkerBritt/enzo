#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include <memory>
#include <vector>

namespace enzo::nt {

class UndoStack
{
public:
    void push(std::unique_ptr<UndoCommand> command);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    void clear();

private:
    std::vector<std::unique_ptr<UndoCommand>> commands_;
    int currentIndex_ = 0;
};

}
