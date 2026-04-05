#pragma once

#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <vector>

namespace enzo::nt {

class ChangeSelectionCommand : public UndoCommand {
  public:
    ChangeSelectionCommand(std::vector<OpId> prev, std::vector<OpId> next)
        : prev_(std::move(prev)), next_(std::move(next)) {}

    void undo() override { nm().setSelectedNodes(prev_); }
    void redo() override { nm().setSelectedNodes(next_); }

  private:
    std::vector<OpId> prev_;
    std::vector<OpId> next_;
};

} // namespace enzo::nt
