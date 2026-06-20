#include "Engine/Dependency/DependencyGraph.h"
#include "Engine/Dependency/Unit.h"
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>
#include <vector>

using namespace enzo;

namespace {
bool contains(const std::vector<dep::Unit>& units, const dep::Unit& target)
{
    for (const dep::Unit& unit : units)
        if (unit == target) return true;
    return false;
}
} // namespace

TEST_CASE("Units with the same fields compare equal")
{
    dep::Unit first{7, "translate", 1};
    dep::Unit second{7, "translate", 1};
    REQUIRE(first == second);
}

TEST_CASE("Units differing in any field compare unequal")
{
    dep::Unit base{7, "translate", 1};
    REQUIRE_FALSE(base == dep::Unit{8, "translate", 1});
    REQUIRE_FALSE(base == dep::Unit{7, "scale", 1});
    REQUIRE_FALSE(base == dep::Unit{7, "translate", 0});
}

TEST_CASE("A unit knows whether it is a node or a parameter")
{
    dep::Unit node{7};
    dep::Unit parameter{7, "scale", 0};

    REQUIRE(node.isNode());
    REQUIRE_FALSE(node.isParameter());
    REQUIRE(parameter.isParameter());
    REQUIRE_FALSE(parameter.isNode());
}

TEST_CASE("A unit keys an unordered map")
{
    std::unordered_map<dep::Unit, int> values;
    values[{7, "translate", 1}] = 42;

    // The same unit reads back the stored value.
    REQUIRE(values.at({7, "translate", 1}) == 42);

    // A different component is a distinct key.
    REQUIRE(values.find({7, "translate", 0}) == values.end());
}

TEST_CASE("Cook order places a dependency before the node that reads it")
{
    dep::DependencyGraph graph;
    graph.addWiredEdge(1, 2);
    graph.addWiredEdge(2, 3);

    std::vector<nt::OpId> order = graph.getCookOrder(3);
    REQUIRE(order == std::vector<nt::OpId>{1, 2, 3});
}

TEST_CASE("Cook order cooks a shared dependency once")
{
    // Node 1 feeds both 2 and 3, which both feed 4.
    dep::DependencyGraph graph;
    graph.addWiredEdge(1, 2);
    graph.addWiredEdge(1, 3);
    graph.addWiredEdge(2, 4);
    graph.addWiredEdge(3, 4);

    std::vector<nt::OpId> order = graph.getCookOrder(4);
    REQUIRE(order.size() == 4);
    REQUIRE(order.front() == 1);
    REQUIRE(order.back() == 4);
}

TEST_CASE("Cook order reports a cycle")
{
    dep::DependencyGraph graph;
    graph.addWiredEdge(1, 2);
    graph.addWiredEdge(2, 1);

    REQUIRE_THROWS(graph.getCookOrder(1));
}

TEST_CASE("Removing a wired edge drops the dependency")
{
    dep::DependencyGraph graph;
    graph.addWiredEdge(1, 2);
    graph.removeWiredEdge(1, 2);

    REQUIRE(graph.getCookOrder(2) == std::vector<nt::OpId>{2});
}

TEST_CASE("Dependents reach every node downstream of a change")
{
    dep::DependencyGraph graph;
    graph.addWiredEdge(1, 2);
    graph.addWiredEdge(2, 3);

    std::vector<dep::Unit> stale = graph.getDependents(dep::Unit{1});
    REQUIRE(contains(stale, dep::Unit{2}));
    REQUIRE(contains(stale, dep::Unit{3}));
}

TEST_CASE("A captured reference makes a parameter depend on a node")
{
    dep::DependencyGraph graph;
    dep::Unit parameter{2, "scale", 0};
    graph.setCapturedDependencies(parameter, {dep::Unit{1}});

    REQUIRE(contains(graph.getDependents(dep::Unit{1}), parameter));
}

TEST_CASE("Setting captured dependencies replaces the old ones")
{
    dep::DependencyGraph graph;
    dep::Unit parameter{2, "scale", 0};
    graph.setCapturedDependencies(parameter, {dep::Unit{1}});
    graph.setCapturedDependencies(parameter, {dep::Unit{5}});

    REQUIRE_FALSE(contains(graph.getDependents(dep::Unit{1}), parameter));
    REQUIRE(contains(graph.getDependents(dep::Unit{5}), parameter));
}

TEST_CASE("Removing a node forgets its edges")
{
    dep::DependencyGraph graph;
    graph.addWiredEdge(1, 2);
    graph.addWiredEdge(2, 3);
    graph.removeNode(2);

    // Node 2 no longer sits between 1 and 3.
    REQUIRE(graph.getCookOrder(3) == std::vector<nt::OpId>{3});
    REQUIRE(graph.getDependents(dep::Unit{1}).empty());
}
