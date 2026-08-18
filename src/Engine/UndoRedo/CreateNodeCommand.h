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
        typeName_ = node.getType().getFullName();
        path_ = node.getPath();
        position_ = node.getPosition();

        nm().deleteNode(nodeId_);
    }

    void redo() override
    {
        nm().createNodeWithId(
            nodeId_,
            nt::NodeTypeTable::requireNodeType(typeName_),
            path_,
            position_
        );
    }

    UndoCommandType type() const override { return UndoCommandType::CreateNode; }

  private:
    NodeId nodeId_;
    std::string typeName_;
    Path path_;
    Vector2 position_;
};

} // namespace enzo::nt
