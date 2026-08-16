#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/UndoRedo/UndoCommand.h"
#include <string>
#include <vector>

namespace enzo::nt {

class DeleteNodeCommand : public UndoCommand
{

    struct SavedParameter
    {
        std::string name;
        prm::PrmValues values;
    };

  public:
    DeleteNodeCommand(NodeId nodeId) : nodeId_(nodeId)
    {
        Node& node = nm().getNode(nodeId_);
        typeName_ = node.getType().getName();
        position_ = node.getPosition();

        // Save parms
        savedParms_ = std::vector<SavedParameter>();
        for (auto weakPrm : node.getParameters())
        {
            if (auto prm = weakPrm.lock())
            {
                savedParms_.push_back({prm->getName(), prm->getValues()});
            }
        }
    }

    void undo() override
    {
        // Restore node
        auto nodeType = nt::NodeTypeTable::getNodeType(typeName_);
        nm().restoreNode(nodeId_, nodeType.value());

        Node& node = nm().getNode(nodeId_);

        // Restore position
        nm().moveNode(nodeId_, position_, true);

        // Restore parms
        for (const auto& saved : savedParms_)
        {
            if (auto prm = node.getParameter(saved.name).lock())
            {
                prm->setValues(saved.values);
            }
        }
    }

    void redo() override { nm().removeNode(nodeId_, false); }

    UndoCommandType type() const override { return UndoCommandType::DeleteNode; }

  private:
    NodeId nodeId_;
    std::string typeName_;
    Vector2 position_;
    std::vector<SavedParameter> savedParms_;
};

} // namespace enzo::nt
