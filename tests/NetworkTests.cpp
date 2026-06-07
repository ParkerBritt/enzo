#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <memory>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/Core/Types.h"
#include <iostream>

struct NMReset 
{
    NMReset()
    {
        enzo::nt::nm()._reset();
    }
    ~NMReset()
    {
        enzo::nt::nm()._reset();
    }

};

// TODO: fix this init monstrosity
struct OperatorTableInit
{
    OperatorTableInit() { enzo::op::OperatorTable::initPlugins(); }
};
static OperatorTableInit _operatorTableInit;
auto testOpInfoOptional = enzo::op::OperatorTable::getOpInfo("grid");
auto testOpInfo = testOpInfoOptional.value();

TEST_CASE_METHOD(NMReset, "network fixture separation start")
{
    using namespace enzo;
    auto& nm = nt::nm();

    nt::OpId newOpId = nm.createOperator(testOpInfo);
    REQUIRE(newOpId==1);
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
    if(nm.isValidOp(newOpId))
    {
        auto newConnection = std::make_shared<nt::GeometryConnection>(newOpId, 1, newOpId2, 3); 

        auto& inputOp = nm.getGeoOperator(newOpId);
        auto& outputOp = nm.getGeoOperator(newOpId2);

        // set output on the upper operator
        outputOp.addOutputConnection(newConnection);

        // set input on the lower operator
        inputOp.addInputConnection(newConnection);

    }
}

TEST_CASE_METHOD(NMReset, "Undoing a node deletion restores its connections")
{
    using namespace enzo;
    auto& nm = nt::nm();
    nm.undoStack().clear();

    // Build two connected nodes where the upstream output feeds the downstream input
    nt::OpId upstream = nm.createOperator(testOpInfo);
    nt::OpId downstream = nm.createOperator(testOpInfo);
    nt::connectOperators(upstream, 0, downstream, 0);

    // Delete the downstream node
    nm.deleteNode(downstream);
    REQUIRE_FALSE(nm.isValidOp(downstream));

    // Undo must restore the node and its connection without throwing
    nm.undoStack().undo();

    REQUIRE(nm.isValidOp(downstream));
    REQUIRE_FALSE(nm.getGeoOperator(downstream).getInputConnection(0).expired());
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

