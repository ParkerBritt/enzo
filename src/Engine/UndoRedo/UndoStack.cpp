#include "Engine/UndoRedo/UndoStack.h"
#include "Engine/UndoRedo/UndoDisabler.h"

namespace enzo::nt {

void UndoStack::push(std::unique_ptr<UndoCommand> command)
{
    if (UndoDisabler::isBlocked(command->type())) return;

    // Fold into the open transaction so the whole operation undoes as one unit
    if (!openGroups_.empty())
    {
        openGroups_.back()->addCommand(std::move(command));
        return;
    }

    // Truncate any redo history
    commands_.erase(commands_.begin() + currentIndex_, commands_.end());
    commands_.push_back(std::move(command));
    currentIndex_++;
}

void UndoStack::beginGroup() { openGroups_.push_back(std::make_unique<UndoGroup>()); }

void UndoStack::endGroup()
{
    if (openGroups_.empty()) return;

    auto group = std::move(openGroups_.back());
    openGroups_.pop_back();

    // An empty group carries no state, and a nested close routes into its parent
    if (group->isEmpty()) return;
    push(std::move(group));
}

// TODO: Currently the other undo commands (MoveNodeCommand, DeleteNodeCommand, etc.) work
// around creating recursive undo states by calling lower-level functions directly (e.g.
// setPosition instead of moveNode, removeOperator instead of deleteNode). This is
// inconsistent with ChangeConnectionCommand which relies on the UndoDisabler block-all
// in undo()/redo() below. The other commands should probably be updated to use the same
// pattern so users don't get tripped up by the inconsistency.
void UndoStack::undo()
{
    if (!canUndo()) return;
    UndoDisabler blockAll;
    currentIndex_--;
    commands_[currentIndex_]->undo();
}

void UndoStack::redo()
{
    if (!canRedo()) return;
    UndoDisabler blockAll;
    commands_[currentIndex_]->redo();
    currentIndex_++;
}

bool UndoStack::canUndo() const { return currentIndex_ > 0; }

bool UndoStack::canRedo() const { return currentIndex_ < static_cast<int>(commands_.size()); }

void UndoStack::clear()
{
    commands_.clear();
    currentIndex_ = 0;
}

} // namespace enzo::nt
