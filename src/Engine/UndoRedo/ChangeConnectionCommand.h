#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/UndoRedo/UndoCommand.h"

namespace enzo::nt {

class ChangeConnectionCommand : public UndoCommand
{
  public:
    enum class Action
    {
        Connect,
        Disconnect
    };

    ChangeConnectionCommand(
        OpId inputOpId,
        unsigned int inputIndex,
        OpId outputOpId,
        unsigned int outputIndex,
        Action action
    )
        : inputOpId_(inputOpId), inputIndex_(inputIndex), outputOpId_(outputOpId),
          outputIndex_(outputIndex), action_(action)
    {
    }

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
    void connect() { nm().connectNodes(inputOpId_, inputIndex_, outputOpId_, outputIndex_); }

    void disconnect()
    {
        nm().disconnectNodes({inputOpId_, inputIndex_, outputOpId_, outputIndex_});
    }

    OpId inputOpId_;
    unsigned int inputIndex_;
    OpId outputOpId_;
    unsigned int outputIndex_;
    Action action_;
};

} // namespace enzo::nt
