#include "Engine/UndoRedo/UndoStack.h"
#include "Engine/UndoRedo/UndoDisabler.h"

namespace enzo::nt {

void UndoStack::push(std::unique_ptr<UndoCommand> command)
{
    if (UndoDisabler::isBlocked(command->type()))
        return;
    // Truncate any redo history
    commands_.erase(commands_.begin() + currentIndex_, commands_.end());
    commands_.push_back(std::move(command));
    currentIndex_++;
}

void UndoStack::undo()
{
    if(!canUndo()) return;
    currentIndex_--;
    commands_[currentIndex_]->undo();
}

void UndoStack::redo()
{
    if(!canRedo()) return;
    commands_[currentIndex_]->redo();
    currentIndex_++;
}

bool UndoStack::canUndo() const
{
    return currentIndex_ > 0;
}

bool UndoStack::canRedo() const
{
    return currentIndex_ < static_cast<int>(commands_.size());
}

void UndoStack::clear()
{
    commands_.clear();
    currentIndex_ = 0;
}

}
