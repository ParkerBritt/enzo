#include "Engine/Network/NodeTypeTable.h"
#include <stdexcept>

namespace enzo::nt {

const NodeType& NodeTypeTable::addNodeType(NodeType nodeType)
{
    nodeTypeStore_.push_back(std::move(nodeType));
    return nodeTypeStore_.back();
}

const NodeType* NodeTypeTable::getNodeType(const std::string& fullName)
{
    for (const NodeType& nodeType : nodeTypeStore_)
        if (nodeType.getFullName() == fullName) return &nodeType;
    return nullptr;
}

const NodeType& NodeTypeTable::requireNodeType(const std::string& fullName)
{
    const NodeType* nodeType = getNodeType(fullName);
    if (!nodeType) throw std::runtime_error("Couldn't find node type: " + fullName);
    return *nodeType;
}

const std::deque<NodeType>& NodeTypeTable::getData() { return nodeTypeStore_; }

std::deque<NodeType> NodeTypeTable::nodeTypeStore_;

} // namespace enzo::nt
