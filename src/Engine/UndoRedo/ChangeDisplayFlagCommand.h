#pragma once

#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <optional>

namespace enzo::nt {

class ChangeDisplayFlagCommand : public UndoCommand {
  public:
    ChangeDisplayFlagCommand(std::optional<OpId> prev, OpId next)
        : prev_(prev), next_(next) {}

    void undo() override {
        if (prev_.has_value()) {
            if (!nm().isValidOp(*prev_)) return;
            nm().setDisplayOp(*prev_);
        } else {
            nm().clearDisplayFlag();
        }
    }

    void redo() override {
        if (!nm().isValidOp(next_)) return;
        nm().setDisplayOp(next_);
    }

  private:
    std::optional<OpId> prev_;
    OpId next_;
};

} // namespace enzo::nt
