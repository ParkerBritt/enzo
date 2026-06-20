#include "Engine/NetworkGraph/Connection.h"
#include "Engine/NetworkGraph/NetworkGraph.h"
#include "Engine/NetworkGraph/Unit.h"
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>
#include <vector>

using namespace enzo;

namespace {
bool contains(const std::vector<nt::Unit>& units, const nt::Unit& target)
{
    for (const nt::Unit& unit : units)
        if (unit == target) return true;
    return false;
}

// A connection from one node's first output into another node's given input slot.
nt::Connection wire(nt::OpId source, nt::OpId target, unsigned int targetInput = 0)
{
    return nt::Connection{source, 0, target, targetInput};
}
} // namespace

TEST_CASE("Units with the same fields compare equal")
{
    nt::Unit first{7, "translate", 1};
    nt::Unit second{7, "translate", 1};
    REQUIRE(first == second);
}

TEST_CASE("Units differing in any field compare unequal")
{
    nt::Unit base{7, "translate", 1};
    REQUIRE_FALSE(base == nt::Unit{8, "translate", 1});
    REQUIRE_FALSE(base == nt::Unit{7, "scale", 1});
    REQUIRE_FALSE(base == nt::Unit{7, "translate", 0});
}

TEST_CASE("A unit knows whether it is a node or a parameter")
{
    nt::Unit node{7};
    nt::Unit parameter{7, "scale", 0};

    REQUIRE(node.isNode());
    REQUIRE_FALSE(node.isParameter());
    REQUIRE(parameter.isParameter());
    REQUIRE_FALSE(parameter.isNode());
}

TEST_CASE("A unit keys an unordered map")
{
    std::unordered_map<nt::Unit, int> values;
    values[{7, "translate", 1}] = 42;

    // The same unit reads back the stored value.
    REQUIRE(values.at({7, "translate", 1}) == 42);

    // A different component is a distinct key.
    REQUIRE(values.find({7, "translate", 0}) == values.end());
}

TEST_CASE("Cook order places a dependency before the node that reads it")
{
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.connect(wire(2, 3));

    std::vector<nt::OpId> order = graph.getCookOrder(3);
    REQUIRE(order == std::vector<nt::OpId>{1, 2, 3});
}

TEST_CASE("Cook order cooks a shared dependency once")
{
    // Node 1 feeds both 2 and 3, which both feed 4.
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.connect(wire(1, 3));
    graph.connect(wire(2, 4, 0));
    graph.connect(wire(3, 4, 1));

    std::vector<nt::OpId> order = graph.getCookOrder(4);
    REQUIRE(order.size() == 4);
    REQUIRE(order.front() == 1);
    REQUIRE(order.back() == 4);
}

TEST_CASE("Cook order reports a cycle")
{
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.connect(wire(2, 1));

    REQUIRE_THROWS(graph.getCookOrder(1));
}

TEST_CASE("Disconnecting drops the dependency")
{
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.disconnect(wire(1, 2));

    REQUIRE(graph.getCookOrder(2) == std::vector<nt::OpId>{2});
}

TEST_CASE("A node's inputs come back ordered by input slot")
{
    nt::NetworkGraph graph;
    graph.connect(wire(5, 9, 1));
    graph.connect(wire(6, 9, 0));

    std::vector<nt::Connection> inputs = graph.getInputs(9);
    REQUIRE(inputs.size() == 2);
    REQUIRE(inputs[0].targetInput == 0);
    REQUIRE(inputs[0].sourceOp == 6);
    REQUIRE(inputs[1].targetInput == 1);
    REQUIRE(inputs[1].sourceOp == 5);
}

TEST_CASE("A node's outputs list every connection leaving it")
{
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.connect(wire(1, 3));

    REQUIRE(graph.getOutputs(1).size() == 2);
}

TEST_CASE("Two connections between the same pair survive removing one")
{
    // Node 1 feeds node 2 through two separate input slots.
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2, 0));
    graph.connect(wire(1, 2, 1));

    graph.disconnect(wire(1, 2, 0));

    // The second connection still keeps 1 ahead of 2.
    REQUIRE(graph.getCookOrder(2) == std::vector<nt::OpId>{1, 2});
}

TEST_CASE("Dependents reach every node downstream of a change")
{
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.connect(wire(2, 3));

    std::vector<nt::Unit> stale = graph.getDependents(nt::Unit{1});
    REQUIRE(contains(stale, nt::Unit{2}));
    REQUIRE(contains(stale, nt::Unit{3}));
}

TEST_CASE("A captured reference makes a parameter depend on a node")
{
    nt::NetworkGraph graph;
    nt::Unit parameter{2, "scale", 0};
    graph.setCapturedDependencies(parameter, {nt::Unit{1}});

    REQUIRE(contains(graph.getDependents(nt::Unit{1}), parameter));
}

TEST_CASE("Setting captured dependencies replaces the old ones")
{
    nt::NetworkGraph graph;
    nt::Unit parameter{2, "scale", 0};
    graph.setCapturedDependencies(parameter, {nt::Unit{1}});
    graph.setCapturedDependencies(parameter, {nt::Unit{5}});

    REQUIRE_FALSE(contains(graph.getDependents(nt::Unit{1}), parameter));
    REQUIRE(contains(graph.getDependents(nt::Unit{5}), parameter));
}

TEST_CASE("Removing a node forgets its edges")
{
    nt::NetworkGraph graph;
    graph.connect(wire(1, 2));
    graph.connect(wire(2, 3));
    graph.removeNode(2);

    // Node 2 no longer sits between 1 and 3.
    REQUIRE(graph.getCookOrder(3) == std::vector<nt::OpId>{3});
    REQUIRE(graph.getDependents(nt::Unit{1}).empty());
}
