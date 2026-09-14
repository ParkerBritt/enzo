#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Ramp.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>
#include <set>

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
const enzo::nt::NodeType& testNodeType = enzo::nt::NodeTypeTable::requireNodeType("enzo::cube");

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

namespace {

// Sets a sphere node's radius and division counts.
void setSphereShape(enzo::nt::Node& sphere, float radius, int columns, int rows)
{
    for (int axis = 0; axis < 3; ++axis)
        sphere.getParameter("radius").lock()->setFloat(radius, axis);
    sphere.getParameter("columns").lock()->setInt(columns);
    sphere.getParameter("rows").lock()->setInt(rows);
}

// Returns a cooked fracture of a dense sphere around seed points on three nested spheres.
enzo::nt::NodeId addFractureOfSphere(int seedColumns, int seedRows)
{
    using namespace enzo;
    auto& nm = nt::nm();

    const nt::NodeId geometry = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::sphere"));
    setSphereShape(nm.getNode(geometry), 1.0f, 64, 32);

    // Turns each seed sphere by a different angle so their rings do not line up.
    const nt::NodeId seeds = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::merge"));
    const float seedRadii[] = {0.3f, 0.6f, 0.9f};
    for (int shellIndex = 0; shellIndex < 3; ++shellIndex)
    {
        const nt::NodeId shell = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::sphere"));
        nt::Node& shellNode = nm.getNode(shell);
        setSphereShape(shellNode, seedRadii[shellIndex], seedColumns, seedRows);
        shellNode.getParameter("rotate").lock()->setFloat(17.0f * (shellIndex + 1), 0);
        shellNode.getParameter("rotate").lock()->setFloat(29.0f * (shellIndex + 1), 1);
        nm.connectNodes(shell, 0, seeds, 0);
    }

    const nt::NodeId fracture =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cellFracture"));
    nm.connectNodes(geometry, 0, fracture, 0);
    nm.connectNodes(seeds, 0, fracture, 1);
    nm.cook(fracture);
    return fracture;
}

// Prints the point, face, piece and inside face counts of a fracture's output.
void printFractureShape(enzo::nt::NodeId fracture)
{
    using namespace enzo;
    auto& nm = nt::nm();
    const auto mesh = std::dynamic_pointer_cast<const geo::Mesh>(
        nm.getNode(fracture).getOutputPacket(0)->getPrimitive(0)
    );
    REQUIRE(mesh != nullptr);

    attr::AttributeHandleRO<intT> piece(mesh->getAttribByName(attr::AttributeOwner::FACE, "piece"));
    attr::AttributeHandleRO<boolT> inside(
        mesh->getGroupByName(attr::AttributeOwner::FACE, "inside")
    );
    std::set<intT> pieceNumbers;
    int insideFaceCount = 0;
    for (Offset faceOffset = 0; faceOffset < mesh->getNumFaces(); ++faceOffset)
    {
        pieceNumbers.insert(piece.getValue(faceOffset));
        if (inside.getValue(faceOffset)) ++insideFaceCount;
    }

    std::cout << "fracture shape: points " << mesh->getNumPoints() << ", faces "
              << mesh->getNumFaces() << ", pieces " << pieceNumbers.size() << ", inside faces "
              << insideFaceCount << "\n";
}

} // namespace

TEST_CASE_METHOD(NMReset, "Cell fracture")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // Cooks only the fracture each run, since the nodes above it stay clean.
    const nt::NodeId fewSeeds = addFractureOfSphere(4, 3);
    printFractureShape(fewSeeds);
    BENCHMARK("Fracture sphere around 30 seeds")
    {
        nm.getNode(fewSeeds).dirtyNode(false);
        nm.cook(fewSeeds);
    };

    const nt::NodeId manySeeds = addFractureOfSphere(8, 5);
    printFractureShape(manySeeds);
    BENCHMARK("Fracture sphere around 102 seeds")
    {
        nm.getNode(manySeeds).dirtyNode(false);
        nm.cook(manySeeds);
    };

    const nt::NodeId denseSeeds = addFractureOfSphere(25, 13);
    printFractureShape(denseSeeds);
    BENCHMARK("Fracture sphere around 906 seeds")
    {
        nm.getNode(denseSeeds).dirtyNode(false);
        nm.cook(denseSeeds);
    };
}

TEST_CASE_METHOD(NMReset, "Cell fracture of a cube")
{
    using namespace enzo;
    auto& nm = nt::nm();

    // Places every seed on a sphere around the cube, so every halfway plane passes through its
    // center.
    const nt::NodeId cube = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cube"));
    const nt::NodeId seeds = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::sphere"));
    setSphereShape(nm.getNode(seeds), 1.0f, 30, 30);
    const nt::NodeId fracture =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cellFracture"));
    nm.connectNodes(cube, 0, fracture, 0);
    nm.connectNodes(seeds, 0, fracture, 1);
    nm.cook(fracture);
    printFractureShape(fracture);

    BENCHMARK("Fracture cube around 872 seeds on a sphere")
    {
        nm.getNode(fracture).dirtyNode(false);
        nm.cook(fracture);
    };
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
