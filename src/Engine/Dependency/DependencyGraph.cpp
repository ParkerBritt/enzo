#include "Engine/Dependency/DependencyGraph.h"
#include <stdexcept>
#include <string>

namespace enzo::dep {

void DependencyGraph::linkEdge_(const Unit& dependency, const Unit& dependent, DependencyKind kind)
{
    dependents_[dependency].push_back({dependent, kind});
    dependencies_[dependent].push_back({dependency, kind});
}

void DependencyGraph::unlinkEdge_(
    const Unit& dependency,
    const Unit& dependent,
    DependencyKind kind
)
{
    eraseEdge_(dependents_, dependency, {dependent, kind});
    eraseEdge_(dependencies_, dependent, {dependency, kind});
}

void DependencyGraph::eraseEdge_(EdgeMap& side, const Unit& key, const Edge& edge)
{
    auto entry = side.find(key);
    if (entry == side.end()) return;

    std::vector<Edge>& edges = entry->second;
    for (auto it = edges.begin(); it != edges.end(); ++it)
    {
        if (*it == edge)
        {
            edges.erase(it);
            break;
        }
    }

    if (edges.empty()) side.erase(entry);
}

void DependencyGraph::addWiredEdge(nt::OpId dependency, nt::OpId dependent)
{
    linkEdge_(Unit{dependency}, Unit{dependent}, DependencyKind::Wired);
}

void DependencyGraph::removeWiredEdge(nt::OpId dependency, nt::OpId dependent)
{
    unlinkEdge_(Unit{dependency}, Unit{dependent}, DependencyKind::Wired);
}

void DependencyGraph::setCapturedDependencies(
    const Unit& dependent,
    const std::vector<Unit>& dependencies
)
{
    // Collect the parameter's current captured edges
    std::vector<Unit> previous;
    auto entry = dependencies_.find(dependent);
    if (entry != dependencies_.end())
        for (const Edge& edge : entry->second)
            if (edge.kind == DependencyKind::Captured) previous.push_back(edge.unit);

    // Swap them out for the freshly read set
    for (const Unit& dependency : previous)
        unlinkEdge_(dependency, dependent, DependencyKind::Captured);
    for (const Unit& dependency : dependencies)
        linkEdge_(dependency, dependent, DependencyKind::Captured);
}

void DependencyGraph::removeNode(nt::OpId opId)
{
    eraseNode_(dependents_, opId);
    eraseNode_(dependencies_, opId);
}

void DependencyGraph::eraseNode_(EdgeMap& side, nt::OpId opId)
{
    for (auto entry = side.begin(); entry != side.end();)
    {
        // Drop the unit when it belongs to the node
        if (entry->first.opId == opId)
        {
            entry = side.erase(entry);
            continue;
        }

        // Otherwise drop only the edges that reach the node
        std::vector<Edge> kept;
        for (const Edge& edge : entry->second)
            if (edge.unit.opId != opId) kept.push_back(edge);

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

void DependencyGraph::clear()
{
    dependents_.clear();
    dependencies_.clear();
}

std::vector<nt::OpId> DependencyGraph::getCookOrder(nt::OpId target) const
{
    std::vector<nt::OpId> opOrder;
    std::unordered_set<Unit> addedUnits;
    std::unordered_set<Unit> unitsBeingAdded;

    addToCookOrder_(Unit{target}, opOrder, addedUnits, unitsBeingAdded);
    return opOrder;
}

void DependencyGraph::addToCookOrder_(
    const Unit& unit,
    std::vector<nt::OpId>& opOrder,
    std::unordered_set<Unit>& addedUnits,
    std::unordered_set<Unit>& unitsBeingAdded
) const
{
    // Already placed
    if (addedUnits.count(unit)) return;

    // Reaching a unit still being added means it depends on itself
    if (unitsBeingAdded.count(unit))
        throw std::runtime_error("Dependency cycle through node " + std::to_string(unit.opId));
    unitsBeingAdded.insert(unit);

    // Place every wired dependency ahead of this unit
    auto entry = dependencies_.find(unit);
    if (entry != dependencies_.end())
        for (const Edge& edge : entry->second)
            if (edge.kind == DependencyKind::Wired)
                addToCookOrder_(edge.unit, opOrder, addedUnits, unitsBeingAdded);

    // Then place the unit after them
    unitsBeingAdded.erase(unit);
    addedUnits.insert(unit);
    opOrder.push_back(unit.opId);
}

std::vector<Unit> DependencyGraph::getDependents(const Unit& changed) const
{
    std::vector<Unit> result;
    std::unordered_set<Unit> seen;
    seen.insert(changed);

    std::vector<Unit> pending;
    pending.push_back(changed);

    // Follow every edge kind, since a wired or captured reader both go stale
    while (!pending.empty())
    {
        Unit unit = pending.back();
        pending.pop_back();

        auto entry = dependents_.find(unit);
        if (entry == dependents_.end()) continue;

        for (const Edge& edge : entry->second)
        {
            if (seen.count(edge.unit)) continue;

            seen.insert(edge.unit);
            result.push_back(edge.unit);
            pending.push_back(edge.unit);
        }
    }
    return result;
}

} // namespace enzo::dep
