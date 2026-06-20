#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <icecream.hpp>
#include <string>

namespace enzo::nt {

/// @brief Undo step for a parameter edit. Restores the whole parameter from a
/// snapshot so flat values and multiparm instances both round trip.
class ChangeParameterCommand : public UndoCommand
{
  public:
    ChangeParameterCommand(
        enzo::nt::OpId opId,
        std::string paramName,
        ParameterSerializable before,
        ParameterSerializable after
    )
        : opId_(opId), paramName_(paramName), before_(std::move(before)), after_(std::move(after))
    {
    }

    // Flat parameters hand over their value vectors and the snapshot is built here.
    ChangeParameterCommand(
        enzo::nt::OpId opId,
        std::string paramName,
        const enzo::prm::PrmValues& before,
        const enzo::prm::PrmValues& after
    )
        : opId_(opId), paramName_(paramName), before_(toSerializable(paramName, before)),
          after_(toSerializable(paramName, after))
    {
    }

    void undo() override { restore_(before_); }
    void redo() override { restore_(after_); }

    UndoCommandType type() const override { return UndoCommandType::ChangeParameter; }

  private:
    void restore_(const ParameterSerializable& snapshot)
    {
        IC(opId_, paramName_);
        if (!nm().isValidOp(opId_))
        {
            IC("ChangeParameterCommand — operator not found", opId_);
            return;
        }
        if (auto prm = nm().getGeoOperator(opId_).getParameter(paramName_).lock())
            applySerializable(*prm, snapshot);
        else
            IC("ChangeParameterCommand — parameter not found", opId_, paramName_);
    }

    enzo::nt::OpId opId_;
    std::string paramName_;
    ParameterSerializable before_;
    ParameterSerializable after_;
};

} // namespace enzo::nt
