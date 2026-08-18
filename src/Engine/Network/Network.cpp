#include "Engine/Network/Network.h"
#include <stdexcept>
#include <string>

namespace enzo {

nt::Node& nt::Network::getNode(nt::NodeId nodeId)
{
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end())
    {
        throw std::out_of_range(
            "NodeId: " + std::to_string(nodeId) + " > max nodeId: " + std::to_string(maxNodeId_) +
            "\n"
        );
    }
    return *it->second;
}

bool nt::Network::isValidNode(nt::NodeId nodeId) const
{
    auto it = nodes_.find(nodeId);
    return it != nodes_.end() && it->second != nullptr;
}

nt::Node* nt::Network::getNodeAtPath(const Path& path)
{
    if (path.isEmpty()) return nullptr;

    for (auto& [nodeId, node] : nodes_)
        if (node->getPath() == path.getString()) return node.get();
    return nullptr;
}

std::vector<nt::NodeId> nt::Network::getChildNodeIds(const Path& scope)
{
    std::vector<NodeId> children;
    for (auto& [nodeId, node] : nodes_)
        if (node->getPath().getParent() == scope.getString()) children.push_back(nodeId);
    return children;
}

void nt::Network::addNode(nt::NodeId nodeId, std::unique_ptr<Node> node)
{
    const nt::NodeType& nodeType = node->getType();
    if (nodeType.hasChildScope()) addScope(node->getPath(), nodeType.childScopeType);

    nodes_.emplace(nodeId, std::move(node));

    if (nodeId > maxNodeId_) maxNodeId_ = nodeId;
}

void nt::Network::eraseNode(nt::NodeId nodeId)
{
    if (!isValidNode(nodeId)) return;

    // Delete scope if it has one
    eraseScope(getNode(nodeId).getPath());

    graph_.removeNode(nodeId);
    nodes_.erase(nodeId);
}

nt::Scope* nt::Network::getScope(const Path& path)
{
    auto found = scopes_.find(path.getString());
    return found != scopes_.end() ? &found->second : nullptr;
}

void nt::Network::addScope(const Path& path, const std::string& scopeType)
{
    scopes_.emplace(path.getString(), Scope(path, scopeType));
}

void nt::Network::eraseScope(const Path& path) { scopes_.erase(path.getString()); }

void nt::Network::clear()
{
    nodes_.clear();
    graph_.clear();
    scopes_.clear();
    maxNodeId_ = 0;

    // The root is where top level nodes live, so a network always has one
    addScope(Path("/"), "geometry");
}

} // namespace enzo
