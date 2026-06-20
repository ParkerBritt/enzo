#include "Engine/Core/Types.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/Parameter/Ramp.h"
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
auto testOpInfo = enzo::op::OperatorTable::getOpInfo("house").value();

TEST_CASE_METHOD(NMReset, "Network Manager")
{
    using namespace enzo;

    auto& nm = nt::nm();

    nt::OpId startOp = nm.createOperator(testOpInfo);
    nt::OpId prevOp = startOp;
    std::vector<nt::OpId> prevOps;

    for (int k = 0; k < 10; k++)
    {
        for (int i = 0; i < 4; ++i)
        {
            nt::OpId newOp = nm.createOperator(testOpInfo);
            prevOps.push_back(newOp);
            nt::nm().connectNodes(newOp, i, prevOp, 0);
        }
        for (int j = 0; j < 10; j++)
        {
            std::vector<nt::OpId> prevOpsBuffer = prevOps;
            for (int i = 0; i < size(prevOpsBuffer); ++i)
            {
                prevOps.clear();
                nt::OpId newOp = nm.createOperator(testOpInfo);
                prevOps.push_back(newOp);
                nt::nm().connectNodes(newOp, 0, prevOpsBuffer[i], 0);
            }
        }
    }

    BENCHMARK("Cook 100 Ops") { nm.setDisplayOp(startOp); };
}

TEST_CASE("Ramp sampling")
{
    using namespace enzo;

    // A curved run bordered by linear keys, the shape the per point hotpath sees.
    prm::Ramp ramp(
        std::vector<prm::Ramp::Key>{
            {0.0f, 0.0f, prm::Interpolation::LINEAR},
            {0.2f, 1.0f, prm::Interpolation::BSPLINE},
            {0.4f, 0.0f, prm::Interpolation::BSPLINE},
            {0.6f, 2.0f, prm::Interpolation::BSPLINE},
            {0.8f, 1.0f, prm::Interpolation::LINEAR},
            {1.0f, 0.0f, prm::Interpolation::LINEAR},
        }
    );

    BENCHMARK("Sample b spline ramp 10k points")
    {
        floatT total = 0;
        for (int i = 0; i < 10000; ++i)
            total += ramp.sample(i / 10000.0f);
        return total;
    };
}
