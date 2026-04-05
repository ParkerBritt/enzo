#pragma once

#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"
#include "Engine/UndoRedo/UndoCommand.h"

namespace enzo::nt {

class ChangeParameterCommand : public UndoCommand {
  public:
    ChangeParameterCommand(enzo::prm::Parameter &parameter, enzo::prm::PrmValues before,
                           enzo::prm::PrmValues after)
        : parameter_(parameter), before_(before), after_(after) {}

    void undo() override { parameter_.setValues(before_); }

    void redo() override { parameter_.setValues(after_); }

  private:
    enzo::prm::Parameter &parameter_;
    enzo::prm::PrmValues before_;
    enzo::prm::PrmValues after_;
};

} // namespace enzo::nt
