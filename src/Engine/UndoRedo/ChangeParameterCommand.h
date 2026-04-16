#pragma once

#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <icecream.hpp>
#include <string>

namespace enzo::nt {

class ChangeParameterCommand : public UndoCommand {
  public:
    ChangeParameterCommand(enzo::nt::OpId opId, std::string paramName, enzo::prm::PrmValues before,
                           enzo::prm::PrmValues after)
        : opId_(opId), paramName_(paramName), before_(before), after_(after) {}

    void undo() override {
        IC(opId_, paramName_);
        if (!nm().isValidOp(opId_)) {
            IC("ChangeParameterCommand::undo — operator not found", opId_);
            return;
        }
        if (auto prm = nm().getGeoOperator(opId_).getParameter(paramName_).lock()) {
            prm->setValues(before_);
        } else {
            IC("ChangeParameterCommand::undo — parameter not found", opId_, paramName_);
        }
    }

    void redo() override {
        IC(opId_, paramName_);
        if (!nm().isValidOp(opId_)) {
            IC("ChangeParameterCommand::redo — operator not found", opId_);
            return;
        }
        if (auto prm = nm().getGeoOperator(opId_).getParameter(paramName_).lock()) {
            prm->setValues(after_);
        } else {
            IC("ChangeParameterCommand::redo — parameter not found", opId_, paramName_);
        }
    }

    UndoCommandType type() const override { return UndoCommandType::ChangeParameter; }

  private:
    enzo::nt::OpId opId_;
    std::string paramName_;
    enzo::prm::PrmValues before_;
    enzo::prm::PrmValues after_;
};

} // namespace enzo::nt
