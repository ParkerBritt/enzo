#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeSnapshot.h"
#include "Engine/UndoRedo/UndoCommand.h"

namespace enzo::nt {

class CreateNodeCommand : public UndoCommand
{
  public:
    CreateNodeCommand(NodeId nodeId) : nodeId_(nodeId) {}

    void undo() override
    {
        snapshot_ = NodeSnapshot::capture(nm().getNode(nodeId_));
        nm().deleteNode(nodeId_);
    }

    void redo() override { snapshot_.restore(nodeId_); }

    UndoCommandType type() const override { return UndoCommandType::CreateNode; }

  private:
    NodeId nodeId_;
    NodeSnapshot snapshot_;
};

} // namespace enzo::nt
