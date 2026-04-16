#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <memory>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/Types.h"
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

