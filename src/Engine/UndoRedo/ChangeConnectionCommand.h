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
        NodeId inputNodeId,
        unsigned int inputIndex,
        NodeId outputNodeId,
        unsigned int outputIndex,
        Action action
    )
        : inputNodeId_(inputNodeId), inputIndex_(inputIndex), outputNodeId_(outputNodeId),
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
    void connect() { nm().connectNodes(inputNodeId_, inputIndex_, outputNodeId_, outputIndex_); }

    void disconnect()
    {
        nm().disconnectNodes({inputNodeId_, inputIndex_, outputNodeId_, outputIndex_});
    }

    NodeId inputNodeId_;
    unsigned int inputIndex_;
    NodeId outputNodeId_;
    unsigned int outputIndex_;
    Action action_;
};

} // namespace enzo::nt
