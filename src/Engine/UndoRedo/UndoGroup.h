#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include <memory>
#include <vector>

namespace enzo::nt {

/// @brief Bundles several commands into a single atomic undo unit.
class UndoGroup : public UndoCommand
{
  public:
    UndoGroup() {}

    /// @brief Appends a command to the group, taking ownership.
    void addCommand(std::unique_ptr<UndoCommand> command)
    {
        commands_.push_back(std::move(command));
    }

    /// @brief Returns true when the group holds no commands.
    bool isEmpty() const { return commands_.empty(); }

    void undo() override
    {
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it)
        {
            (*it)->undo();
        }
    }

    void redo() override
    {
        for (auto& command : commands_)
        {
            command->redo();
        }
    }

    UndoCommandType type() const override { return UndoCommandType::UndoGroup; }

  private:
    std::vector<std::unique_ptr<UndoCommand>> commands_;
};

} // namespace enzo::nt
