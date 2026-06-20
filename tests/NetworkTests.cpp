#include "Engine/Core/Types.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
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
struct OperatorTableInit
{
    OperatorTableInit() { enzo::op::OperatorTable::initPlugins(); }
};
static OperatorTableInit _operatorTableInit;
auto testOpInfoOptional = enzo::op::OperatorTable::getOpInfo("grid");
auto testOpInfo = testOpInfoOptional.value();
auto transformOpInfo = enzo::op::OperatorTable::getOpInfo("transform").value();

TEST_CASE_METHOD(NMReset, "network fixture separation start")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::OpId newOpId = nm.createOperator(testOpInfo);
    REQUIRE(newOpId == 1);
    REQUIRE(nm.isValidOp(1));
}

TEST_CASE_METHOD(NMReset, "network fixture separation end")
{
    using namespace enzo;
    auto& nm = nt::nm();

    REQUIRE_FALSE(nm.isValidOp(1));
}

TEST_CASE_METHOD(NMReset, "network")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::OpId newOpId = nm.createOperator(testOpInfo);
    nt::OpId newOpId2 = nm.createOperator(testOpInfo);

    REQUIRE(nm.isValidOp(newOpId));
    REQUIRE(nm.isValidOp(newOpId2));

    nm.connectNodes(newOpId, 0, newOpId2, 0);
    REQUIRE(nm.graph().getInputConnection(newOpId2, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Undoing a node deletion restores its connections")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    // Build two connected nodes where the upstream output feeds the downstream input
    nt::OpId upstream = nm.createOperator(testOpInfo);
    nt::OpId downstream = nm.createOperator(testOpInfo);
    nt::nm().connectNodes(upstream, 0, downstream, 0);

    // Delete the downstream node
    nm.deleteNode(downstream);
    REQUIRE_FALSE(nm.isValidOp(downstream));

    // Undo must restore the node and its connection without throwing
    nm.undoStack().undo();

    REQUIRE(nm.isValidOp(downstream));
    REQUIRE(nm.graph().getInputConnection(downstream, 0).has_value());
}

TEST_CASE_METHOD(NMReset, "Cooking a node cooks its whole upstream chain")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // Wire a three node chain where each output feeds the next input
    nt::OpId first = nm.createOperator(testOpInfo);
    nt::OpId second = nm.createOperator(testOpInfo);
    nt::OpId third = nm.createOperator(testOpInfo);
    nt::nm().connectNodes(first, 0, second, 0);
    nt::nm().connectNodes(second, 0, third, 0);

    nm.cookOp(third);

    REQUIRE_FALSE(nm.getGeoOperator(first).isDirty());
    REQUIRE_FALSE(nm.getGeoOperator(second).isDirty());
    REQUIRE_FALSE(nm.getGeoOperator(third).isDirty());
}

TEST_CASE_METHOD(NMReset, "Dirtying an upstream node restages everything downstream")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::OpId first = nm.createOperator(testOpInfo);
    nt::OpId second = nm.createOperator(testOpInfo);
    nt::OpId third = nm.createOperator(testOpInfo);
    nt::nm().connectNodes(first, 0, second, 0);
    nt::nm().connectNodes(second, 0, third, 0);

    // Start from a fully cooked chain
    nm.cookOp(third);

    // A change at the top must mark the whole chain below it stale
    nm.getGeoOperator(first).dirtyNode();

    REQUIRE(nm.getGeoOperator(second).isDirty());
    REQUIRE(nm.getGeoOperator(third).isDirty());
}

TEST_CASE_METHOD(NMReset, "Cooking pulls geometry across an input connection")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // A grid feeding a transform, which copies the grid's primitives through
    nt::OpId grid = nm.createOperator(testOpInfo);
    nt::OpId transform = nm.createOperator(transformOpInfo);
    nt::nm().connectNodes(grid, 0, transform, 0);

    nm.cookOp(transform);

    size_t gridSize = nm.getGeoOperator(grid).getOutputPacket(0)->size();
    size_t transformSize = nm.getGeoOperator(transform).getOutputPacket(0)->size();

    REQUIRE(gridSize > 0);
    REQUIRE(transformSize == gridSize);
}

TEST_CASE_METHOD(NMReset, "A node with no input cooks to an empty output")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // A transform with nothing wired in has no primitives to pass through
    nt::OpId transform = nm.createOperator(transformOpInfo);

    nm.cookOp(transform);

    REQUIRE(nm.getGeoOperator(transform).getOutputPacket(0)->size() == 0);
}

TEST_CASE_METHOD(NMReset, "reset")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::OpId newOpId = nm.createOperator(testOpInfo);

    nm._reset();

    REQUIRE_FALSE(nm.isValidOp(newOpId));

    nt::OpId newOpId2 = nm.createOperator(testOpInfo);
    REQUIRE(nm.isValidOp(newOpId2));
}
