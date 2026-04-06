#pragma once

#include "UndoCommands.h"
#include <algorithm>
#include <vector>

class UndoDisabler {
  public:
    UndoDisabler(UndoCommandType command) : blockedCommand_(command) {
        blockedCommands_.push_back(command);
    }
    ~UndoDisabler() {
        auto it = std::find(blockedCommands_.begin(), blockedCommands_.end(), blockedCommand_);
        blockedCommands_.erase(it);
    }

    UndoDisabler(const UndoDisabler &) = delete;
    UndoDisabler &operator=(const UndoDisabler &) = delete;

    static bool isBlocked(UndoCommandType command) {
        return std::find(blockedCommands_.begin(), blockedCommands_.end(), command) !=
               blockedCommands_.end();
    }

  private:
    UndoCommandType blockedCommand_;
    static inline std::vector<UndoCommandType> blockedCommands_;
};
