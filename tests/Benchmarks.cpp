#include "Engine/Core/Types.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeTypeTable.h"
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
struct NodeTypeTableInit
{
    NodeTypeTableInit() { enzo::nt::NodeTypeTable::initPlugins(); }
};
static NodeTypeTableInit _nodeTypeTableInit;
auto testNodeType = enzo::nt::NodeTypeTable::getNodeType("cube").value();

TEST_CASE_METHOD(NMReset, "Network Manager")
{
    using namespace enzo;

    auto& nm = nt::nm();

    nt::NodeId startNode = nm.createNode(testNodeType);
    nt::NodeId prevNode = startNode;
    std::vector<nt::NodeId> prevNodes;

    for (int k = 0; k < 10; k++)
    {
        for (int i = 0; i < 4; ++i)
        {
            nt::NodeId newNode = nm.createNode(testNodeType);
            prevNodes.push_back(newNode);
            nt::nm().connectNodes(newNode, i, prevNode, 0);
        }
        for (int j = 0; j < 10; j++)
        {
            std::vector<nt::NodeId> prevNodesBuffer = prevNodes;
            for (int i = 0; i < size(prevNodesBuffer); ++i)
            {
                prevNodes.clear();
                nt::NodeId newNode = nm.createNode(testNodeType);
                prevNodes.push_back(newNode);
                nt::nm().connectNodes(newNode, 0, prevNodesBuffer[i], 0);
            }
        }
    }

    BENCHMARK("Cook 100 Nodes") { nm.setDisplayNode(startNode); };
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
