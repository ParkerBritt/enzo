#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NetworkPath.h"
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
const enzo::nt::NodeType& testNodeType = enzo::nt::NodeTypeTable::requireNodeType("enzo::grid");
const enzo::nt::NodeType& transformNodeType =
    enzo::nt::NodeTypeTable::requireNodeType("enzo::transform");

// A node type that holds a scope. It lives here rather than in the node type table, which
// the loader test expects to hold only the shipped node types.
const enzo::nt::NodeType containerNodeType = [] {
    enzo::nt::NodeType nodeType = enzo::nt::NodeTypeTable::requireNodeType("enzo::grid");
    nodeType.internalName = "container";
    nodeType.childScopeType = "geometry";
    return nodeType;
}();

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

TEST_CASE_METHOD(NMReset, "New nodes are numbered from their type name")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId first = nm.createNode(testNodeType);
    nt::NodeId second = nm.createNode(testNodeType);
    nt::NodeId transform = nm.createNode(transformNodeType);

    REQUIRE(nm.getNode(first).getPath() == "/grid1");
    REQUIRE(nm.getNode(second).getPath() == "/grid2");

    // Each type is numbered on its own, so a transform starts again at one
    REQUIRE(nm.getNode(transform).getPath() == "/transform1");
}

TEST_CASE_METHOD(NMReset, "A requested node name is numbered until it is free")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId first = nm.createNode(testNodeType, Path("/"), "mesh");
    nt::NodeId second = nm.createNode(testNodeType, Path("/"), "mesh");

    REQUIRE(nm.getNode(first).getName() == "mesh");
    REQUIRE(nm.getNode(second).getName() == "mesh1");
}

TEST_CASE_METHOD(NMReset, "A node name only has to be free among its siblings")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId atRoot = nm.createNode(testNodeType);
    nm.createNode(containerNodeType);
    nt::NodeId nested = nm.createNode(testNodeType, Path("/container1"));

    // The same name in two scopes is two different nodes
    REQUIRE(nm.getNode(atRoot).getPath() == "/grid1");
    REQUIRE(nm.getNode(nested).getPath() == "/container1/grid1");
    REQUIRE(atRoot != nested);
}

TEST_CASE_METHOD(NMReset, "A node is found by an absolute or a relative path")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId outer = nm.createNode(testNodeType);
    nm.createNode(containerNodeType);
    nt::NodeId inner = nm.createNode(testNodeType, Path("/container1"));
    nt::NodeId sibling = nm.createNode(testNodeType, Path("/container1"));

    // An absolute path needs no node to start from
    REQUIRE(nm.findNode("/grid1") == &nm.getNode(outer));
    REQUIRE(nm.findNode("/container1/grid2") == &nm.getNode(sibling));

    // A bare name is read from the scope holding the asking node
    REQUIRE(nm.findNode("grid2", inner) == &nm.getNode(sibling));

    // A parent step leaves the scope the asking node sits in
    REQUIRE(nm.findNode("../grid1", inner) == &nm.getNode(outer));

    // An empty path names the asking node itself
    REQUIRE(nm.findNode("", inner) == &nm.getNode(inner));

    // A top level node reads its relative paths from the root
    REQUIRE(nm.findNode("grid1", outer) == &nm.getNode(outer));
    REQUIRE(nm.findNode("container1/grid1", outer) == &nm.getNode(inner));

    // A name matching nothing in the asking node's scope is not found elsewhere
    REQUIRE(nm.findNode("nothing_here", inner) == nullptr);
}

TEST_CASE_METHOD(NMReset, "Undoing a node deletion restores its name")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nm.createNode(testNodeType, Path("/"), "mesh");
    nt::NodeId second = nm.createNode(testNodeType, Path("/"), "mesh");
    REQUIRE(nm.getNode(second).getName() == "mesh1");

    nm.deleteNode(second);
    nm.undoStack().undo();

    // Expressions reference nodes by name, so the name has to come back unchanged
    REQUIRE(nm.getNode(second).getName() == "mesh1");
}

TEST_CASE_METHOD(NMReset, "A scope exists only where a node holds one")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // The root scope is always there for the top level nodes to live in
    REQUIRE(nm.getScope(Path("/")) != nullptr);

    nm.createNode(testNodeType);
    nm.createNode(containerNodeType);

    REQUIRE(nm.getScope(Path("/container1")) != nullptr);
    REQUIRE(nm.getScope(Path("/container1"))->getType() == "geometry");

    // An ordinary node holds nothing, so there is nowhere inside it to go
    REQUIRE(nm.getScope(Path("/grid1")) == nullptr);
    REQUIRE(nm.getScope(Path("/nothing_here")) == nullptr);
}

TEST_CASE_METHOD(NMReset, "A node cannot be created where there is no scope")
{
    using namespace enzo;
    auto& nm = nt::nm();

    REQUIRE_THROWS(nm.createNode(testNodeType, Path("/nothing_here")));

    // An ordinary node is not a place other nodes can live
    nm.createNode(testNodeType);
    REQUIRE_THROWS(nm.createNode(testNodeType, Path("/grid1")));
}

TEST_CASE_METHOD(NMReset, "A scope lists the nodes directly inside it")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId atRoot = nm.createNode(testNodeType);
    nt::NodeId container = nm.createNode(containerNodeType);
    nt::NodeId inner = nm.createNode(testNodeType, Path("/container1"));

    std::vector<nt::NodeId> rootChildren = nm.getChildNodeIds(Path("/"));
    REQUIRE(rootChildren.size() == 2);
    REQUIRE(std::find(rootChildren.begin(), rootChildren.end(), atRoot) != rootChildren.end());
    REQUIRE(std::find(rootChildren.begin(), rootChildren.end(), container) != rootChildren.end());

    // A node nested deeper belongs to its own scope, not to the one above
    REQUIRE(nm.getChildNodeIds(Path("/container1")) == std::vector<nt::NodeId>{inner});
}

TEST_CASE_METHOD(NMReset, "Deleting a node that holds a scope takes its contents with it")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId container = nm.createNode(containerNodeType);
    nt::NodeId inner = nm.createNode(testNodeType, Path("/container1"));
    nt::NodeId deeper = nm.createNode(containerNodeType, Path("/container1"));
    nt::NodeId deepest = nm.createNode(testNodeType, Path("/container1/container1"));

    nm.deleteNode(container);

    // Nothing is left behind at any depth
    REQUIRE_FALSE(nm.isValidNode(inner));
    REQUIRE_FALSE(nm.isValidNode(deeper));
    REQUIRE_FALSE(nm.isValidNode(deepest));

    // The scopes go with the nodes that held them
    REQUIRE(nm.getScope(Path("/container1")) == nullptr);
    REQUIRE(nm.getScope(Path("/container1/container1")) == nullptr);
    REQUIRE(nm.getChildNodeIds(Path("/")).empty());
}
