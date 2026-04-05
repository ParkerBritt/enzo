#pragma once

#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"
#include "Engine/UndoRedo/UndoCommand.h"

namespace enzo::nt {

class UndoGroup : public UndoCommand {
  public:
    UndoGroup() {}

    void addCommand(nt::UndoCommand &command) { commands_.push_back(command); }

    void undo() override {
        for (auto it = commands_.end(); it != commands_.begin(); --it) {
            it->undo();
        }
    }

    void redo() override {
        for (auto it = commands_.begin(); it != commands_.end(); ++it) {
            it->redo();
        }
    }

  private:
    std::vector<UndoCommand> commands_;
};

} // namespace enzo::nt
