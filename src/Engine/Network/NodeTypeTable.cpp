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

const NodeAlias& NodeTypeTable::addNodeAlias(NodeAlias nodeAlias)
{
    const std::string fullName = nodeAlias.getFullName();
    if (getNodeType(fullName) || getNodeAlias(fullName))
        throw std::runtime_error("Couldn't add alias " + fullName + ", the name is taken");

    nodeAliasStore_.push_back(std::move(nodeAlias));
    return nodeAliasStore_.back();
}

const NodeAlias* NodeTypeTable::getNodeAlias(const std::string& fullName)
{
    for (const NodeAlias& nodeAlias : nodeAliasStore_)
        if (nodeAlias.getFullName() == fullName) return &nodeAlias;
    return nullptr;
}

const std::deque<NodeAlias>& NodeTypeTable::getNodeAliases() { return nodeAliasStore_; }

std::deque<NodeType> NodeTypeTable::nodeTypeStore_;
std::deque<NodeAlias> NodeTypeTable::nodeAliasStore_;

} // namespace enzo::nt
