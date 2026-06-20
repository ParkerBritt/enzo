#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/UndoRedo/UndoGroup.h"
#include <memory>
#include <vector>

namespace enzo::nt {

class UndoStack
{
  public:
    /// @brief Records a command, or folds it into the open group during a transaction.
    void push(std::unique_ptr<UndoCommand> command);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    void clear();

    /// @brief Opens a transaction so subsequent pushes collect into one atomic group.
    void beginGroup();

    /// @brief Closes the innermost transaction, recording its commands as a single group.
    void endGroup();

  private:
    std::vector<std::unique_ptr<UndoCommand>> commands_;
    std::vector<std::unique_ptr<UndoGroup>> openGroups_;
    int currentIndex_ = 0;
};

/// @brief RAII helper that opens an undo group for the duration of its scope.
class UndoTransaction
{
  public:
    explicit UndoTransaction(UndoStack& stack) : stack_(stack) { stack_.beginGroup(); }
    ~UndoTransaction() { stack_.endGroup(); }

    UndoTransaction(const UndoTransaction&) = delete;
    UndoTransaction& operator=(const UndoTransaction&) = delete;

  private:
    UndoStack& stack_;
};

} // namespace enzo::nt
