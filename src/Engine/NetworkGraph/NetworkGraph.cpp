#include "Engine/NetworkGraph/NetworkGraph.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace enzo::nt {

void NetworkGraph::connect(const Connection& connection)
{
    byTarget_[connection.targetOp].push_back(connection);
    bySource_[connection.sourceOp].push_back(connection);
}

void NetworkGraph::disconnect(const Connection& connection)
{
    eraseConnection_(byTarget_, connection.targetOp, connection);
    eraseConnection_(bySource_, connection.sourceOp, connection);
}

void NetworkGraph::eraseConnection_(ConnectionMap& side, OpId key, const Connection& connection)
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

std::vector<Connection> NetworkGraph::getInputs(OpId target) const
{
    auto entry = byTarget_.find(target);
    if (entry == byTarget_.end()) return {};

    std::vector<Connection> inputs = entry->second;
    std::sort(inputs.begin(), inputs.end(), orderByInputSlot);
    return inputs;
}

std::optional<Connection>
NetworkGraph::getInputConnection(OpId target, unsigned int inputSlot) const
{
    auto entry = byTarget_.find(target);
    if (entry == byTarget_.end()) return std::nullopt;

    for (const Connection& connection : entry->second)
        if (connection.targetInput == inputSlot) return connection;

    return std::nullopt;
}

std::vector<Connection> NetworkGraph::getOutputs(OpId source) const
{
    auto entry = bySource_.find(source);
    if (entry == bySource_.end()) return {};
    return entry->second;
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

void NetworkGraph::removeNode(OpId opId)
{
    eraseConnectionsTouching_(byTarget_, opId);
    eraseConnectionsTouching_(bySource_, opId);
    eraseCapturedTouching_(capturedDependents_, opId);
    eraseCapturedTouching_(capturedDependencies_, opId);
}

void NetworkGraph::eraseConnectionsTouching_(ConnectionMap& side, OpId opId)
{
    for (auto entry = side.begin(); entry != side.end();)
    {
        // Drop the whole list when it belongs to the node
        if (entry->first == opId)
        {
            entry = side.erase(entry);
            continue;
        }

        // Otherwise keep only the connections that do not name the node
        std::vector<Connection> kept;
        for (const Connection& connection : entry->second)
            if (connection.sourceOp != opId && connection.targetOp != opId)
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

void NetworkGraph::eraseCapturedTouching_(CapturedMap& map, OpId opId)
{
    for (auto entry = map.begin(); entry != map.end();)
    {
        // Drop the whole list when its unit belongs to the node
        if (entry->first.opId == opId)
        {
            entry = map.erase(entry);
            continue;
        }

        // Otherwise keep only the units that do not belong to the node
        std::vector<Unit> kept;
        for (const Unit& unit : entry->second)
            if (unit.opId != opId) kept.push_back(unit);

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

std::vector<OpId> NetworkGraph::getCookOrder(OpId target) const
{
    std::vector<OpId> opOrder;
    std::unordered_set<OpId> addedOps;
    std::unordered_set<OpId> opsBeingAdded;

    addToCookOrder_(target, opOrder, addedOps, opsBeingAdded);
    return opOrder;
}

void NetworkGraph::addToCookOrder_(
    OpId opId,
    std::vector<OpId>& opOrder,
    std::unordered_set<OpId>& addedOps,
    std::unordered_set<OpId>& opsBeingAdded
) const
{
    // Already placed
    if (addedOps.count(opId)) return;

    // Reaching a node still being added means it depends on itself
    if (opsBeingAdded.count(opId))
        throw std::runtime_error("Dependency cycle through node " + std::to_string(opId));
    opsBeingAdded.insert(opId);

    // Place every node feeding an input ahead of this one
    auto entry = byTarget_.find(opId);
    if (entry != byTarget_.end())
        for (const Connection& connection : entry->second)
            addToCookOrder_(connection.sourceOp, opOrder, addedOps, opsBeingAdded);

    // Then place this node after them
    opsBeingAdded.erase(opId);
    addedOps.insert(opId);
    opOrder.push_back(opId);
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
        auto outputs = bySource_.find(unit.opId);
        if (outputs != bySource_.end())
            for (const Connection& connection : outputs->second)
                addDependent_(Unit{connection.targetOp}, dependents, seen, pending);

        // Captured readers are the units whose expressions read this one
        auto captured = capturedDependents_.find(unit);
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
