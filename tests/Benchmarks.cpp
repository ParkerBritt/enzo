#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <memory>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Types.h"
#include "Engine/Operator/OperatorTable.h"
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
auto testOpInfo = enzo::op::OperatorTable::getOpInfo("house").value();

TEST_CASE_METHOD(NMReset, "Network Manager")
{
    using namespace enzo;

    auto& nm = nt::nm();

    nt::OpId startOp = nm.createOperator(testOpInfo);
    nt::OpId prevOp = startOp;
    std::vector<nt::OpId> prevOps;

    for(int k=0; k<10; k++)
    {
        for(int i=0; i<4; ++i)
        {
            nt::OpId newOp = nm.createOperator(testOpInfo);
            prevOps.push_back(newOp);
            nt::connectOperators(newOp, i, prevOp, 0);
        }
        for(int j=0; j<10; j++)
        {
            std::vector<nt::OpId> prevOpsBuffer = prevOps;
            for(int i=0; i<size(prevOpsBuffer); ++i)
            {
                prevOps.clear();
                nt::OpId newOp = nm.createOperator(testOpInfo);
                prevOps.push_back(newOp);
                nt::connectOperators(newOp, 0, prevOpsBuffer[i], 0);

                
            }
        }
    }

    BENCHMARK("Cook 100 Ops")
    {
        nm.setDisplayOp(startOp);
    };
    

}
