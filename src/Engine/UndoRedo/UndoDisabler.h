#pragma once

#include "UndoCommands.h"
#include <algorithm>
#include <vector>

class UndoDisabler {
  public:
    UndoDisabler(UndoCommandType command) {
        blockedCommands_.push_back(command);
        blockedCommand_ = command;
    }
    ~UndoDisabler() {
        // Find current command
        auto it = std::find(blockedCommands_.begin(), blockedCommands_.end(), blockedCommand_);
        // Erase current command
        blockedCommands_.erase(it);
    }

  private:
    UndoCommandType blockedCommand_;
    static std::vector<UndoCommandType> blockedCommands_;
};
