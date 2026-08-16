#include "Engine/NetworkGraph/NetworkGraph.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace enzo::nt {

void NetworkGraph::connect(const Connection& connection)
{
    byTarget_[connection.targetNode].push_back(connection);
    bySource_[connection.sourceNode].push_back(connection);
}

void NetworkGraph::disconnect(const Connection& connection)
{
    eraseConnection_(byTarget_, connection.targetNode, connection);
    eraseConnection_(bySource_, connection.sourceNode, connection);
}

void NetworkGraph::eraseConnection_(ConnectionMap& side, NodeId key, const Connection& connection)
{
    auto entry = side.find(key);
    if (entry == side.end()) return;

    std::vector<Connection>& connections = entry->second;
    for (auto it = connections.begin(); it != connections.end(); ++it)
    {
        if (*it == connection)
        {
            connections.erase(it);
            break;
        }
    }

    if (connections.empty()) side.erase(entry);
}

namespace {
bool orderByInputSlot(const Connection& first, const Connection& second)
{
    return first.targetInput < second.targetInput;
}
} // namespace

std::vector<Connection> NetworkGraph::getInputs(NodeId target) const
{
    auto entry = byTarget_.find(target);
    if (entry == byTarget_.end()) return {};

    std::vector<Connection> inputs = entry->second;
    std::sort(inputs.begin(), inputs.end(), orderByInputSlot);
    return inputs;
}

std::optional<Connection>
NetworkGraph::getInputConnection(NodeId target, unsigned int inputSlot) const
{
    auto entry = byTarget_.find(target);
    if (entry == byTarget_.end()) return std::nullopt;

    for (const Connection& connection : entry->second)
        if (connection.targetInput == inputSlot) return connection;

    return std::nullopt;
}

std::vector<Connection> NetworkGraph::getOutputs(NodeId source) const
{
    auto entry = bySource_.find(source);
    if (entry == bySource_.end()) return {};
    return entry->second;
}

std::vector<Connection> NetworkGraph::getConnections() const
{
    std::vector<Connection> connections;
    for (const auto& [source, outgoing] : bySource_)
        connections.insert(connections.end(), outgoing.begin(), outgoing.end());
    return connections;
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

void NetworkGraph::removeNode(NodeId nodeId)
{
    eraseConnectionsTouching_(byTarget_, nodeId);
    eraseConnectionsTouching_(bySource_, nodeId);
    eraseCapturedTouching_(capturedDependents_, nodeId);
    eraseCapturedTouching_(capturedDependencies_, nodeId);
}

void NetworkGraph::eraseConnectionsTouching_(ConnectionMap& side, NodeId nodeId)
{
    for (auto entry = side.begin(); entry != side.end();)
    {
        // Drop the whole list when it belongs to the node
        if (entry->first == nodeId)
        {
            entry = side.erase(entry);
            continue;
        }

        // Otherwise keep only the connections that do not name the node
        std::vector<Connection> kept;
        for (const Connection& connection : entry->second)
            if (connection.sourceNode != nodeId && connection.targetNode != nodeId)
                kept.push_back(connection);

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
        for (const Connection& connection : entry->second)
            addToCookOrder_(connection.sourceNode, nodeOrder, addedNodes, nodesBeingAdded);

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
            for (const Connection& connection : outputs->second)
                addDependent_(Unit{connection.targetNode}, dependents, seen, pending);

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
