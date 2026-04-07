#pragma once

#include "UndoCommands.h"
#include <algorithm>
#include <optional>
#include <vector>

class UndoDisabler {
  public:
    /// @brief Blocks all undo command types while this object is alive.
    UndoDisabler() : blockedCommand_(std::nullopt) {
        blockAllCount_++;
    }

    /// @brief Blocks a specific undo command type while this object is alive.
    UndoDisabler(UndoCommandType command) : blockedCommand_(command) {
        blockedCommands_.push_back(command);
    }

    ~UndoDisabler() {
        if (blockedCommand_) {
            auto it = std::find(blockedCommands_.begin(), blockedCommands_.end(), *blockedCommand_);
            blockedCommands_.erase(it);
        } else {
            blockAllCount_--;
        }
    }

    UndoDisabler(const UndoDisabler &) = delete;
    UndoDisabler &operator=(const UndoDisabler &) = delete;

    static bool isBlocked(UndoCommandType command) {
        return blockAllCount_ > 0 ||
               std::find(blockedCommands_.begin(), blockedCommands_.end(), command) !=
               blockedCommands_.end();
    }

  private:
    std::optional<UndoCommandType> blockedCommand_;
    static inline std::vector<UndoCommandType> blockedCommands_;
    static inline int blockAllCount_ = 0;
};
