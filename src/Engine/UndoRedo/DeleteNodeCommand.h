#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeSnapshot.h"
#include "Engine/UndoRedo/UndoCommand.h"

namespace enzo::nt {

class DeleteNodeCommand : public UndoCommand
{
  public:
    DeleteNodeCommand(NodeId nodeId)
        : nodeId_(nodeId), snapshot_(NodeSnapshot::capture(nm().getNode(nodeId)))
    {
    }

    void undo() override { snapshot_.restore(nodeId_); }

    void redo() override { nm().deleteNode(nodeId_); }

    UndoCommandType type() const override { return UndoCommandType::DeleteNode; }

  private:
    NodeId nodeId_;
    NodeSnapshot snapshot_;
};

} // namespace enzo::nt
