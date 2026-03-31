#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/Types.h"
#include <vector>
#include <string>

namespace enzo::nt {

class DeleteNodeCommand : public UndoCommand
{

struct SavedParameter
{
    std::string name;
    prm::PrmValues values;
};

public:
    DeleteNodeCommand(OpId opId) : opId_(opId)
    {
        GeometryOperator& op = nm().getGeoOperator(opId_);
        typeName_ =  op.getTypeName();
        position_ = op.getPosition();

        // Save parms
        savedParms_ = std::vector<SavedParameter>();
        for(auto weakPrm : op.getParameters())
        {
            if(auto prm = weakPrm.lock())
            {
                savedParms_.push_back({prm->getName(), prm->getValues()});
            }
        }
    }

    void undo() override
    {
        // Restore operator
        auto opInfo = op::OperatorTable::getOpInfo(typeName_);
        nm().restoreOperator(opId_, opInfo.value());

        GeometryOperator& op = nm().getGeoOperator(opId_);

        // Restore position
        nm().moveNode(opId_, position_, true);

        // Restore parms
        for(const auto& saved : savedParms_)
        {
            if(auto prm = op.getParameter(saved.name).lock())
            {
                prm->setValues(saved.values);
            }
        }
    }

    void redo() override
    {
        nm().removeOperator(opId_);
    }

private:
    OpId opId_;
    std::string typeName_;
    bt::Vector2f position_;
    std::vector<SavedParameter> savedParms_;
};

}
