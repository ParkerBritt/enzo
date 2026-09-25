#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NetworkPath.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeAlias.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>
#include <vector>

struct NMReset
{
    NMReset()
    {
        enzo::nt::NodeLoader::loadNodes();
        enzo::nt::nm()._reset();
    }
    ~NMReset() { enzo::nt::nm()._reset(); }
};

// Registers a node type that holds a scope and returns its full name.
std::string addContainerType()
{
    enzo::nt::NodeLoader::loadNodes();
    enzo::nt::NodeType nodeType = enzo::nt::NodeTypeTable::requireNodeType("enzo::grid");
    nodeType.internalName = "container";
    nodeType.childScopeType = "geometry";
    return enzo::nt::NodeTypeTable::addNodeType(std::move(nodeType)).getFullName();
}

const std::string containerTypeName = addContainerType();

// Registers an alias of the grid that starts with three rows and returns its full name.
std::string addThreeRowGridAlias()
{
    enzo::nt::NodeLoader::loadNodes();

    ParameterSerializable rows;
    rows.name = "rows";
    rows.intValues = {3};

    enzo::nt::NodeAlias alias;
    alias.internalName = "threeRowGrid";
    alias.typeNamespace = "test";
    alias.displayName = "Three Row Grid";
    alias.aliasedType = "enzo::grid";
    alias.parameterValues = {rows};
    return enzo::nt::NodeTypeTable::addNodeAlias(std::move(alias)).getFullName();
}

const std::string threeRowGridAliasName = addThreeRowGridAlias();

TEST_CASE_METHOD(NMReset, "network fixture separation start")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId newNodeId = nm.createNode("enzo::grid");
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

    nt::NodeId newNodeId = nm.createNode("enzo::grid");
    nt::NodeId newNodeId2 = nm.createNode("enzo::grid");

    REQUIRE(nm.isValidNode(newNodeId));
    REQUIRE(nm.isValidNode(newNodeId2));

    nm.connectNodes(newNodeId, 0, newNodeId2, 0);
    REQUIRE(nm.graph().getInputNodeLink(newNodeId2, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Undoing a node deletion restores its node links")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    // Build two connected nodes where the upstream output feeds the downstream input
    nt::NodeId upstream = nm.createNode("enzo::grid");
    nt::NodeId downstream = nm.createNode("enzo::grid");
    nt::nm().connectNodes(upstream, 0, downstream, 0);

    // Delete the downstream node
    nm.deleteNode(downstream);
    REQUIRE_FALSE(nm.isValidNode(downstream));

    // Undo must restore the node and its node link without throwing
    nm.undoStack().undo();

    REQUIRE(nm.isValidNode(downstream));
    REQUIRE(nm.graph().getInputNodeLink(downstream, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Cooking a node cooks its whole upstream chain")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // Wire a three node chain where each output feeds the next input
    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");
    nt::NodeId third = nm.createNode("enzo::grid");
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

    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");
    nt::NodeId third = nm.createNode("enzo::grid");
    nt::nm().connectNodes(first, 0, second, 0);
    nt::nm().connectNodes(second, 0, third, 0);

    // Start from a fully cooked chain
    nm.cook(third);

    // A change at the top must mark the whole chain below it stale
    nm.getNode(first).dirtyNode();

    REQUIRE(nm.getNode(second).isDirty());
    REQUIRE(nm.getNode(third).isDirty());
}

TEST_CASE_METHOD(NMReset, "Cooking pulls geometry across an input node link")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // A grid feeding a transform, which copies the grid's primitives through
    nt::NodeId grid = nm.createNode("enzo::grid");
    nt::NodeId transform = nm.createNode("enzo::transform");
    nt::nm().connectNodes(grid, 0, transform, 0);

    nm.cook(transform);

    size_t gridSize = nm.getNode(grid).getOutputPacket(0)->size();
    size_t transformSize = nm.getNode(transform).getOutputPacket(0)->size();

    REQUIRE(gridSize > 0);
    REQUIRE(transformSize == gridSize);
}

TEST_CASE_METHOD(NMReset, "Cooking an output reaches a node nothing is wired to")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // Nothing is wired to the grid, so naming it is the only way to its geometry
    nt::NodeId grid = nm.createNode("enzo::grid");

    NodePacket packet = nm.cookOutput(grid, 0);

    REQUIRE_FALSE(nm.getNode(grid).isDirty());
    REQUIRE(packet.size() > 0);
}

TEST_CASE_METHOD(NMReset, "A cooked output is a copy the caller owns")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId grid = nm.createNode("enzo::grid");

    NodePacket packet = nm.cookOutput(grid, 0);
    size_t originalSize = packet.size();
    packet.removePrim(packet.getPrimitives().front()->getPath());

    // Taking a primitive out of the copy must leave the node's own output untouched
    REQUIRE(packet.size() == originalSize - 1);
    REQUIRE(nm.getNode(grid).getOutputPacket(0)->size() == originalSize);
}

TEST_CASE_METHOD(NMReset, "A node with no input cooks to an empty output")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // A transform with nothing wired in has no primitives to pass through
    nt::NodeId transform = nm.createNode("enzo::transform");

    nm.cook(transform);

    REQUIRE(nm.getNode(transform).getOutputPacket(0)->size() == 0);
}

TEST_CASE_METHOD(NMReset, "reset")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId newNodeId = nm.createNode("enzo::grid");

    nm._reset();

    REQUIRE_FALSE(nm.isValidNode(newNodeId));

    nt::NodeId newNodeId2 = nm.createNode("enzo::grid");
    REQUIRE(nm.isValidNode(newNodeId2));
}

TEST_CASE_METHOD(NMReset, "New nodes are numbered from their type name")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");
    nt::NodeId transform = nm.createNode("enzo::transform");

    REQUIRE(nm.getNode(first).getPath() == "/grid1");
    REQUIRE(nm.getNode(second).getPath() == "/grid2");

    // Each type is numbered on its own, so a transform starts again at one
    REQUIRE(nm.getNode(transform).getPath() == "/transform1");
}

TEST_CASE_METHOD(NMReset, "A requested node name is numbered until it is free")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId first = nm.createNode("enzo::grid", Path("/"), "mesh");
    nt::NodeId second = nm.createNode("enzo::grid", Path("/"), "mesh");

    REQUIRE(nm.getNode(first).getName() == "mesh");
    REQUIRE(nm.getNode(second).getName() == "mesh1");
}

TEST_CASE_METHOD(NMReset, "A node name only has to be free among its siblings")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId atRoot = nm.createNode("enzo::grid");
    nm.createNode(containerTypeName);
    nt::NodeId nested = nm.createNode("enzo::grid", Path("/container1"));

    // The same name in two scopes is two different nodes
    REQUIRE(nm.getNode(atRoot).getPath() == "/grid1");
    REQUIRE(nm.getNode(nested).getPath() == "/container1/grid1");
    REQUIRE(atRoot != nested);
}

TEST_CASE_METHOD(NMReset, "A node is found by an absolute or a relative path")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId outer = nm.createNode("enzo::grid");
    nm.createNode(containerTypeName);
    nt::NodeId inner = nm.createNode("enzo::grid", Path("/container1"));
    nt::NodeId sibling = nm.createNode("enzo::grid", Path("/container1"));

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

    nm.createNode("enzo::grid", Path("/"), "mesh");
    nt::NodeId second = nm.createNode("enzo::grid", Path("/"), "mesh");
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

    nm.createNode("enzo::grid");
    nm.createNode(containerTypeName);

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

    REQUIRE_THROWS(nm.createNode("enzo::grid", Path("/nothing_here")));

    // An ordinary node is not a place other nodes can live
    nm.createNode("enzo::grid");
    REQUIRE_THROWS(nm.createNode("enzo::grid", Path("/grid1")));
}

TEST_CASE_METHOD(NMReset, "A scope lists the nodes directly inside it")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId atRoot = nm.createNode("enzo::grid");
    nt::NodeId container = nm.createNode(containerTypeName);
    nt::NodeId inner = nm.createNode("enzo::grid", Path("/container1"));

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

    nt::NodeId container = nm.createNode(containerTypeName);
    nt::NodeId inner = nm.createNode("enzo::grid", Path("/container1"));
    nt::NodeId deeper = nm.createNode(containerTypeName, Path("/container1"));
    nt::NodeId deepest = nm.createNode("enzo::grid", Path("/container1/container1"));

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

TEST_CASE_METHOD(NMReset, "Undoing a delete brings the node back with its position and wiring")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    nt::NodeId upstream = nm.createNode("enzo::grid");
    nt::NodeId downstream = nm.createNode("enzo::transform", Path("/"), "", {5.f, 7.f});
    nm.connectNodes(upstream, 0, downstream, 0);

    nm.deleteNode(downstream);
    REQUIRE_FALSE(nm.isValidNode(downstream));

    // The node returns under the id and name it had, so expressions naming it still resolve
    nm.undoStack().undo();
    REQUIRE(nm.isValidNode(downstream));
    REQUIRE(nm.getNode(downstream).getPath() == "/transform1");
    REQUIRE(nm.getNode(downstream).getPosition().x() == 5.f);
    REQUIRE(nm.getNode(downstream).getPosition().y() == 7.f);
    REQUIRE(nm.graph().getInputNodeLink(downstream, 0).has_value());

    // Redo takes it away again, and the stack is still good for another round trip
    nm.undoStack().redo();
    REQUIRE_FALSE(nm.isValidNode(downstream));

    nm.undoStack().undo();
    REQUIRE(nm.isValidNode(downstream));
    REQUIRE(nm.graph().getInputNodeLink(downstream, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Undoing a delete restores a scope before the nodes living in it")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    nt::NodeId container = nm.createNode(containerTypeName);
    nt::NodeId inner = nm.createNode("enzo::grid", Path("/container1"));
    nt::NodeId deeper = nm.createNode("enzo::transform", Path("/container1"), "", {2.f, 3.f});
    nm.connectNodes(inner, 0, deeper, 0);

    nm.deleteNode(container);
    REQUIRE(nm.getChildNodeIds(Path("/")).empty());

    // The container comes back first, so its contents have a scope to return to
    nm.undoStack().undo();
    REQUIRE(nm.isValidNode(container));
    REQUIRE(nm.getScope(Path("/container1")) != nullptr);
    REQUIRE(nm.isValidNode(inner));
    REQUIRE(nm.isValidNode(deeper));
    REQUIRE(nm.getChildNodeIds(Path("/container1")).size() == 2);
    REQUIRE(nm.getNode(deeper).getPosition().x() == 2.f);
    REQUIRE(nm.graph().getInputNodeLink(deeper, 0).has_value());

    // Redo empties it out again at every depth
    nm.undoStack().redo();
    REQUIRE_FALSE(nm.isValidNode(container));
    REQUIRE_FALSE(nm.isValidNode(inner));
    REQUIRE_FALSE(nm.isValidNode(deeper));
    REQUIRE(nm.getScope(Path("/container1")) == nullptr);
}

// Returns the source node feeding each input of a node, low index first.
static std::vector<enzo::nt::NodeId> getInputSources(enzo::nt::NodeId nodeId)
{
    std::vector<enzo::nt::NodeId> sources;
    for (const enzo::nt::NodeLink& nodeLink : enzo::nt::nm().graph().getInputs(nodeId))
        sources.push_back(nodeLink.sourceNode);
    return sources;
}

TEST_CASE_METHOD(NMReset, "Connecting into a multi input port makes room for the node link")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId merge = nm.createNode("enzo::merge");
    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");

    nm.connectNodes(first, 0, merge, 0);
    nm.connectNodes(second, 0, merge, 1);
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{first, second});

    // Connects into an input the multi port already holds, moving the rest up
    nt::NodeId inserted = nm.createNode("enzo::grid");
    nm.connectNodes(inserted, 0, merge, 1);
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{first, inserted, second});
}

TEST_CASE_METHOD(NMReset, "Disconnecting from a multi input port closes the gap")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId merge = nm.createNode("enzo::merge");
    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");
    nt::NodeId third = nm.createNode("enzo::grid");

    nm.connectNodes(first, 0, merge, 0);
    nm.connectNodes(second, 0, merge, 1);
    nm.connectNodes(third, 0, merge, 2);

    nm.disconnectNodes({second, 0, merge, 1});
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{first, third});
}

TEST_CASE_METHOD(NMReset, "Undoing a node link into a multi input port restores the order")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    nt::NodeId merge = nm.createNode("enzo::merge");
    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");
    nm.connectNodes(first, 0, merge, 0);
    nm.connectNodes(second, 0, merge, 1);

    nt::NodeId inserted = nm.createNode("enzo::grid");
    nm.connectNodes(inserted, 0, merge, 0);
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{inserted, first, second});

    nm.undoStack().undo();
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{first, second});

    nm.undoStack().redo();
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{inserted, first, second});
}

TEST_CASE_METHOD(NMReset, "Connecting into an occupied single input port replaces the node link")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId transform = nm.createNode("enzo::transform");
    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");

    nm.connectNodes(first, 0, transform, 0);
    nm.connectNodes(second, 0, transform, 0);
    REQUIRE(getInputSources(transform) == std::vector<nt::NodeId>{second});
}

TEST_CASE_METHOD(NMReset, "Connecting past the last input of a multi input port appends")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId merge = nm.createNode("enzo::merge");
    nt::NodeId first = nm.createNode("enzo::grid");
    nt::NodeId second = nm.createNode("enzo::grid");

    nm.connectNodes(first, 0, merge, 0);
    nt::NodeLink nodeLink = nm.connectNodes(second, 0, merge, 5);

    REQUIRE(nodeLink.targetInput == 1);
    REQUIRE(getInputSources(merge) == std::vector<nt::NodeId>{first, second});
    REQUIRE(nm.getInputCount(merge) == 2);
}

TEST_CASE_METHOD(
    NMReset,
    "An alias creates a node of the type it stands in for with its values set"
)
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::NodeId grid = nm.createNode(threeRowGridAliasName);
    nt::Node& node = nm.getNode(grid);

    REQUIRE(node.getType().getFullName() == "enzo::grid");
    REQUIRE(node.getPath() == "/grid1");
    REQUIRE(node.getParameter("rows").lock()->evalInt() == 3);
    REQUIRE(node.getParameter("columns").lock()->evalInt() == 10);
}

TEST_CASE_METHOD(NMReset, "An alias leaves the defaults of the type it stands in for alone")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nm.createNode(threeRowGridAliasName);
    nt::NodeId plainGrid = nm.createNode("enzo::grid");

    REQUIRE(nm.getNode(plainGrid).getParameter("rows").lock()->evalInt() == 10);
}

TEST_CASE_METHOD(NMReset, "Redoing the creation of an alias node brings back its values")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    nt::NodeId grid = nm.createNode(threeRowGridAliasName);

    nm.undoStack().undo();
    REQUIRE_FALSE(nm.isValidNode(grid));

    nm.undoStack().redo();
    REQUIRE(nm.getNode(grid).getParameter("rows").lock()->evalInt() == 3);
}
