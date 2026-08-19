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

nt::Node& nt::Network::createNode(nt::NodeId nodeId, const nt::NodeType& nodeType, const Path& path)
{
    if (nodeType.hasChildScope()) createScope(path, nodeType.childScopeType);

    std::unique_ptr<Node>& storedNode = nodes_[nodeId];
    storedNode = std::make_unique<Node>(nodeId, nodeType, path);

    if (nodeId > maxNodeId_) maxNodeId_ = nodeId;

    return *storedNode;
}

void nt::Network::deleteNode(nt::NodeId nodeId)
{
    if (!isValidNode(nodeId)) return;

    deleteScope(getNode(nodeId).getPath());
    graph_.removeNode(nodeId);
    nodes_.erase(nodeId);
}

nt::Scope* nt::Network::getScope(const Path& path)
{
    auto found = scopes_.find(path.getString());
    return found != scopes_.end() ? &found->second : nullptr;
}

void nt::Network::createScope(const Path& path, const std::string& scopeType)
{
    scopes_.emplace(path.getString(), Scope(path, scopeType));
}

void nt::Network::deleteScope(const Path& path) { scopes_.erase(path.getString()); }

void nt::Network::clear()
{
    nodes_.clear();
    graph_.clear();
    scopes_.clear();
    maxNodeId_ = 0;

    // The root is where top level nodes live, so a network always has one
    createScope(Path("/"), "geometry");
}

nt::Node* nt::Network::findNode(const NetworkPath& path, NodeId fromNode)
{
    NetworkPath nodePath = path.getNode();

    // An empty node path names the node the lookup starts from.
    if (nodePath.isEmpty()) return isValidNode(fromNode) ? &getNode(fromNode) : nullptr;

    // A relative path is read from the scope holding the asking node
    Path anchor = isValidNode(fromNode) ? getNode(fromNode).getPath().getParent() : Path("/");

    return getNodeAtPath(nodePath.makeAbsoluteFrom(anchor));
}

std::weak_ptr<prm::NodeParameter>
nt::Network::findParameter(const NetworkPath& path, NodeId fromNode)
{
    if (!path.hasParameter()) return {};

    Node* node = findNode(path, fromNode);
    if (!node) return {};

    return node->getParameter(path.getParameter());
}

} // namespace enzo
