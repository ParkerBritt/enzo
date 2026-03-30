#pragma once

#include "Engine/UndoRedo/UndoCommand.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/Types.h"
#include <vector>
#include <string>

namespace enzo::nt {

struct SavedParameter
{
    std::string name;
    prm::PrmValues values;
};

class DeleteNodeCommand : public UndoCommand
{
public:
    DeleteNodeCommand(OpId opId, std::string typeName, bt::Vector2f position,
                      std::vector<SavedParameter> savedParms)
        : opId_(opId), typeName_(std::move(typeName)), position_(position),
          savedParms_(std::move(savedParms)) {}

    void undo() override
    {
        auto opInfo = op::OperatorTable::getOpInfo(typeName_);
        nm().restoreOperator(opId_, opInfo.value(), position_);

        GeometryOperator& op = nm().getGeoOperator(opId_);
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
