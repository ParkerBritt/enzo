#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <icecream.hpp>
#include <string>
#include <vector>

namespace enzo::nt {

class CreateNodeCommand : public UndoCommand
{
  public:
    CreateNodeCommand(NodeId nodeId) : nodeId_(nodeId) {}

    void undo() override
    {
        Node& node = nm().getNode(nodeId_);
        typeName_ = node.getType().getName();
        position_ = node.getPosition();

        nm().removeNode(nodeId_, false);
    }

    void redo() override
    {
        // Restore node
        auto nodeType = nt::NodeTypeTable::getNodeType(typeName_);
        nm().restoreNode(nodeId_, nodeType.value());

        // Restore position
        Node& node = nm().getNode(nodeId_);
        nm().moveNode(nodeId_, position_, true);
    }

    UndoCommandType type() const override { return UndoCommandType::CreateNode; }

  private:
    NodeId nodeId_;
    std::string typeName_;
    Vector2 position_;
};

} // namespace enzo::nt
