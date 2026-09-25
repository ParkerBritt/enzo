#include "Engine/NetworkGraph/NetworkGraph.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace enzo::nt {

void NetworkGraph::connect(const NodeLink& nodeLink)
{
    byTarget_[nodeLink.targetNode].push_back(nodeLink);
    bySource_[nodeLink.sourceNode].push_back(nodeLink);
}

void NetworkGraph::disconnect(const NodeLink& nodeLink)
{
    eraseNodeLink_(byTarget_, nodeLink.targetNode, nodeLink);
    eraseNodeLink_(bySource_, nodeLink.sourceNode, nodeLink);
}

void NetworkGraph::eraseNodeLink_(NodeLinkMap& side, NodeId key, const NodeLink& nodeLink)
{
    auto entry = side.find(key);
    if (entry == side.end()) return;

    std::vector<NodeLink>& nodeLinks = entry->second;
    for (auto it = nodeLinks.begin(); it != nodeLinks.end(); ++it)
    {
        if (*it == nodeLink)
        {
            nodeLinks.erase(it);
            break;
        }
    }

    if (nodeLinks.empty()) side.erase(entry);
}

namespace {
bool orderByInputIndex(const NodeLink& first, const NodeLink& second)
{
    return first.targetInput < second.targetInput;
}
} // namespace

std::vector<NodeLink> NetworkGraph::getInputs(NodeId target) const
{
    auto entry = byTarget_.find(target);
    if (entry == byTarget_.end()) return {};

    std::vector<NodeLink> inputs = entry->second;
    std::sort(inputs.begin(), inputs.end(), orderByInputIndex);
    return inputs;
}

std::optional<NodeLink>
NetworkGraph::getInputNodeLink(NodeId target, unsigned int inputIndex) const
{
    auto entry = byTarget_.find(target);
    if (entry == byTarget_.end()) return std::nullopt;

    for (const NodeLink& nodeLink : entry->second)
        if (nodeLink.targetInput == inputIndex) return nodeLink;

    return std::nullopt;
}

std::vector<NodeLink> NetworkGraph::getOutputs(NodeId source) const
{
    auto entry = bySource_.find(source);
    if (entry == bySource_.end()) return {};
    return entry->second;
}

std::vector<NodeLink> NetworkGraph::getNodeLinks() const
{
    std::vector<NodeLink> nodeLinks;
    for (const auto& [source, outgoing] : bySource_)
        nodeLinks.insert(nodeLinks.end(), outgoing.begin(), outgoing.end());
    return nodeLinks;
}

void NetworkGraph::setCapturedDependencies(
    const Unit& dependent,
    const std::vector<Unit>& dependencies
)
{
    // Drop the parameter's previous reads from both maps
    auto previous = capturedDependencies_.find(dependent);
    if (previous != capturedDependencies_.end())
    {
        for (const Unit& dependency : previous->second)
            eraseUnit_(capturedDependents_, dependency, dependent);
        capturedDependencies_.erase(previous);
    }

    if (dependencies.empty()) return;

    // Record the freshly read set both ways
    capturedDependencies_[dependent] = dependencies;
    for (const Unit& dependency : dependencies)
        capturedDependents_[dependency].push_back(dependent);
}

void NetworkGraph::eraseUnit_(CapturedMap& map, const Unit& key, const Unit& value)
{
    auto entry = map.find(key);
    if (entry == map.end()) return;

    std::vector<Unit>& units = entry->second;
    for (auto it = units.begin(); it != units.end(); ++it)
    {
        if (*it == value)
        {
            units.erase(it);
            break;
        }
    }

    if (units.empty()) map.erase(entry);
}

void NetworkGraph::setTimeDependent(const Unit& dependent, bool dependsOnTime)
{
    if (dependsOnTime)
        timeDependents_.insert(dependent);
    else
        timeDependents_.erase(dependent);
}

std::vector<Unit> NetworkGraph::getTimeDependents() const
{
    std::vector<Unit> dependents(timeDependents_.begin(), timeDependents_.end());
    std::unordered_set<Unit> seen(timeDependents_.begin(), timeDependents_.end());

    for (const Unit& reader : timeDependents_)
        for (const Unit& dependent : getDependents(reader))
            if (seen.insert(dependent).second) dependents.push_back(dependent);

    return dependents;
}

void NetworkGraph::removeNode(NodeId nodeId)
{
    eraseNodeLinksTouching_(byTarget_, nodeId);
    eraseNodeLinksTouching_(bySource_, nodeId);
    eraseCapturedTouching_(capturedDependents_, nodeId);
    eraseCapturedTouching_(capturedDependencies_, nodeId);
    std::erase_if(timeDependents_, [nodeId](const Unit& unit) { return unit.nodeId == nodeId; });
}

void NetworkGraph::eraseNodeLinksTouching_(NodeLinkMap& side, NodeId nodeId)
{
    for (auto entry = side.begin(); entry != side.end();)
    {
        // Drop the whole list when it belongs to the node
        if (entry->first == nodeId)
        {
            entry = side.erase(entry);
            continue;
        }

        // Otherwise keep only the node links that do not name the node
        std::vector<NodeLink> kept;
        for (const NodeLink& nodeLink : entry->second)
            if (nodeLink.sourceNode != nodeId && nodeLink.targetNode != nodeId)
                kept.push_back(nodeLink);

        if (kept.empty())
        {
            entry = side.erase(entry);
        }
        else
        {
            entry->second = std::move(kept);
            ++entry;
        }
    }
}

void NetworkGraph::eraseCapturedTouching_(CapturedMap& map, NodeId nodeId)
{
    for (auto entry = map.begin(); entry != map.end();)
    {
        // Drop the whole list when its unit belongs to the node
        if (entry->first.nodeId == nodeId)
        {
            entry = map.erase(entry);
            continue;
        }

        // Otherwise keep only the units that do not belong to the node
        std::vector<Unit> kept;
        for (const Unit& unit : entry->second)
            if (unit.nodeId != nodeId) kept.push_back(unit);

        if (kept.empty())
        {
            entry = map.erase(entry);
        }
        else
        {
            entry->second = std::move(kept);
            ++entry;
        }
    }
}

void NetworkGraph::clear()
{
    byTarget_.clear();
    bySource_.clear();
    capturedDependents_.clear();
    capturedDependencies_.clear();
    timeDependents_.clear();
}

std::vector<NodeId> NetworkGraph::getCookOrder(NodeId target) const
{
    std::vector<NodeId> nodeOrder;
    std::unordered_set<NodeId> addedNodes;
    std::unordered_set<NodeId> nodesBeingAdded;

    addToCookOrder_(target, nodeOrder, addedNodes, nodesBeingAdded);
    return nodeOrder;
}

void NetworkGraph::addToCookOrder_(
    NodeId nodeId,
    std::vector<NodeId>& nodeOrder,
    std::unordered_set<NodeId>& addedNodes,
    std::unordered_set<NodeId>& nodesBeingAdded
) const
{
    // Already placed
    if (addedNodes.count(nodeId)) return;

    // Reaching a node still being added means it depends on itself
    if (nodesBeingAdded.count(nodeId))
        throw std::runtime_error("Dependency cycle through node " + std::to_string(nodeId));
    nodesBeingAdded.insert(nodeId);

    // Place every node feeding an input ahead of this one
    auto entry = byTarget_.find(nodeId);
    if (entry != byTarget_.end())
        for (const NodeLink& nodeLink : entry->second)
            addToCookOrder_(nodeLink.sourceNode, nodeOrder, addedNodes, nodesBeingAdded);

    // Then place this node after them
    nodesBeingAdded.erase(nodeId);
    addedNodes.insert(nodeId);
    nodeOrder.push_back(nodeId);
}

std::vector<Unit> NetworkGraph::getDependents(const Unit& changed) const
{
    std::vector<Unit> dependents;
    std::unordered_set<Unit> seen;
    seen.insert(changed);

    std::vector<Unit> pending;
    pending.push_back(changed);

    while (!pending.empty())
    {
        Unit unit = pending.back();
        pending.pop_back();

        // Wired readers are the nodes fed by this node's outputs
        auto outputs = bySource_.find(unit.nodeId);
        if (outputs != bySource_.end())
            for (const NodeLink& nodeLink : outputs->second)
                addDependent_(Unit{nodeLink.targetNode}, dependents, seen, pending);

        // Captured readers are the units whose expressions read this node
        auto captured = capturedDependents_.find(Unit{unit.nodeId});
        if (captured != capturedDependents_.end())
            for (const Unit& reader : captured->second)
                addDependent_(reader, dependents, seen, pending);
    }
    return dependents;
}

void NetworkGraph::addDependent_(
    const Unit& unit,
    std::vector<Unit>& dependents,
    std::unordered_set<Unit>& seen,
    std::vector<Unit>& pending
)
{
    if (seen.count(unit)) return;

    seen.insert(unit);
    dependents.push_back(unit);
    pending.push_back(unit);
}

} // namespace enzo::nt
