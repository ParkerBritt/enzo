#include "Engine/Network/NodeSnapshot.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeTypeTable.h"

namespace enzo {

nt::NodeSnapshot nt::NodeSnapshot::capture(nt::NodeId nodeId)
{
    Node& node = nm().getNode(nodeId);

    NodeSnapshot snapshot;
    snapshot.typeName_ = node.getType().getFullName();
    snapshot.path_ = node.getPath();
    snapshot.position_ = node.getPosition();

    for (auto weakParameter : node.getParameters())
        if (auto parameter = weakParameter.lock())
            snapshot.parameters_.push_back(toSerializable(*parameter));

    return snapshot;
}

void nt::NodeSnapshot::restore(nt::NodeId nodeId) const
{
    nm().createNodeWithId(nodeId, NodeTypeTable::requireNodeType(typeName_), path_, position_);

    Node& node = nm().getNode(nodeId);
    for (const ParameterSerializable& parameterModel : parameters_)
        if (auto parameter = node.getParameter(parameterModel.name).lock())
            applySerializable(*parameter, parameterModel);
}

} // namespace enzo
