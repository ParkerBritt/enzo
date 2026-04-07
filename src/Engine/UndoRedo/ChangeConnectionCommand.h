#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Types.h"

namespace enzo::nt {

class ChangeConnectionCommand : public UndoCommand
{
public:
    enum class Action { Connect, Disconnect };

    ChangeConnectionCommand(OpId inputOpId, unsigned int inputIndex,
                            OpId outputOpId, unsigned int outputIndex,
                            Action action)
        : inputOpId_(inputOpId), inputIndex_(inputIndex),
          outputOpId_(outputOpId), outputIndex_(outputIndex),
          action_(action) {}

    void undo() override
    {
        if (action_ == Action::Connect)
            disconnect();
        else
            connect();
    }

    void redo() override
    {
        if (action_ == Action::Connect)
            connect();
        else
            disconnect();
    }

    UndoCommandType type() const override { return UndoCommandType::ChangeConnection; }

private:
    void connect()
    {
        connectOperators(inputOpId_, inputIndex_, outputOpId_, outputIndex_);
    }

    void disconnect()
    {
        auto& outputOp = nm().getGeoOperator(outputOpId_);
        if (auto conn = outputOp.getInputConnection(outputIndex_).lock())
        {
            conn->remove();
        }
    }

    OpId inputOpId_;
    unsigned int inputIndex_;
    OpId outputOpId_;
    unsigned int outputIndex_;
    Action action_;
};

}
