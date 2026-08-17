#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>

struct NMReset
{
    NMReset() { enzo::nt::nm()._reset(); }
    ~NMReset() { enzo::nt::nm()._reset(); }
};

// TODO: fix this init monstrosity
struct NodeTypeTableInit
{
    NodeTypeTableInit() { enzo::nt::NodeLoader::loadNodes(); }
};
static NodeTypeTableInit _nodeTypeTableInit;
const enzo::nt::NodeType& testNodeType = enzo::nt::NodeTypeTable::requireNodeType("grid");
const enzo::nt::NodeType& transformNodeType = enzo::nt::NodeTypeTable::requireNodeType("transform");

TEST_CASE_METHOD(NMReset, "network fixture separation start")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId newNodeId = nm.createNode(testNodeType);
    REQUIRE(newNodeId == 1);
    REQUIRE(nm.isValidNode(1));
}

TEST_CASE_METHOD(NMReset, "network fixture separation end")
{
    using namespace enzo;
    auto& nm = nt::nm();

    REQUIRE_FALSE(nm.isValidNode(1));
}

TEST_CASE_METHOD(NMReset, "network")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId newNodeId = nm.createNode(testNodeType);
    nt::NodeId newNodeId2 = nm.createNode(testNodeType);

    REQUIRE(nm.isValidNode(newNodeId));
    REQUIRE(nm.isValidNode(newNodeId2));

    nm.connectNodes(newNodeId, 0, newNodeId2, 0);
    REQUIRE(nm.graph().getInputConnection(newNodeId2, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Undoing a node deletion restores its connections")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    // Build two connected nodes where the upstream output feeds the downstream input
    nt::NodeId upstream = nm.createNode(testNodeType);
    nt::NodeId downstream = nm.createNode(testNodeType);
    nt::nm().connectNodes(upstream, 0, downstream, 0);

    // Delete the downstream node
    nm.deleteNode(downstream);
    REQUIRE_FALSE(nm.isValidNode(downstream));

    // Undo must restore the node and its connection without throwing
    nm.undoStack().undo();

    REQUIRE(nm.isValidNode(downstream));
    REQUIRE(nm.graph().getInputConnection(downstream, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Cooking a node cooks its whole upstream chain")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // Wire a three node chain where each output feeds the next input
    nt::NodeId first = nm.createNode(testNodeType);
    nt::NodeId second = nm.createNode(testNodeType);
    nt::NodeId third = nm.createNode(testNodeType);
    nt::nm().connectNodes(first, 0, second, 0);
    nt::nm().connectNodes(second, 0, third, 0);

    nm.cook(third);

    REQUIRE_FALSE(nm.getNode(first).isDirty());
    REQUIRE_FALSE(nm.getNode(second).isDirty());
    REQUIRE_FALSE(nm.getNode(third).isDirty());
}

TEST_CASE_METHOD(NMReset, "Dirtying an upstream node restages everything downstream")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId first = nm.createNode(testNodeType);
    nt::NodeId second = nm.createNode(testNodeType);
    nt::NodeId third = nm.createNode(testNodeType);
    nt::nm().connectNodes(first, 0, second, 0);
    nt::nm().connectNodes(second, 0, third, 0);

    // Start from a fully cooked chain
    nm.cook(third);

    // A change at the top must mark the whole chain below it stale
    nm.getNode(first).dirtyNode();

    REQUIRE(nm.getNode(second).isDirty());
    REQUIRE(nm.getNode(third).isDirty());
}

TEST_CASE_METHOD(NMReset, "Cooking pulls geometry across an input connection")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // A grid feeding a transform, which copies the grid's primitives through
    nt::NodeId grid = nm.createNode(testNodeType);
    nt::NodeId transform = nm.createNode(transformNodeType);
    nt::nm().connectNodes(grid, 0, transform, 0);

    nm.cook(transform);

    size_t gridSize = nm.getNode(grid).getOutputPacket(0)->size();
    size_t transformSize = nm.getNode(transform).getOutputPacket(0)->size();

    REQUIRE(gridSize > 0);
    REQUIRE(transformSize == gridSize);
}

TEST_CASE_METHOD(NMReset, "A node with no input cooks to an empty output")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // A transform with nothing wired in has no primitives to pass through
    nt::NodeId transform = nm.createNode(transformNodeType);

    nm.cook(transform);

    REQUIRE(nm.getNode(transform).getOutputPacket(0)->size() == 0);
}

TEST_CASE_METHOD(NMReset, "reset")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId newNodeId = nm.createNode(testNodeType);

    nm._reset();

    REQUIRE_FALSE(nm.isValidNode(newNodeId));

    nt::NodeId newNodeId2 = nm.createNode(testNodeType);
    REQUIRE(nm.isValidNode(newNodeId2));
}
