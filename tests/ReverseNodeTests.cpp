#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <memory>
#include <span>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// A grid feeding a reverse node.
struct GridAndReverse
{
    nt::NodeId grid;
    nt::NodeId reverse;
};

// Returns the mesh a cooked node put on its first output.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Builds a two quad grid wired into a reverse node.
GridAndReverse addReverseAfterGrid()
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    GridAndReverse nodes;
    nodes.grid = nm.createNode("enzo::grid");
    nodes.reverse = nm.createNode("enzo::reverse");
    nm.connectNodes(nodes.grid, 0, nodes.reverse, 0);

    nm.getNode(nodes.grid).getParameter("rows").lock()->setInt(2);
    nm.getNode(nodes.grid).getParameter("columns").lock()->setInt(3);

    return nodes;
}

void requireFaceReversed(const geo::Mesh& reversed, const geo::Mesh& original, Offset faceOffset)
{
    const std::span<const intT> reversedPoints = reversed.getFacePoints(faceOffset);
    const std::span<const intT> originalPoints = original.getFacePoints(faceOffset);
    REQUIRE(reversedPoints.size() == originalPoints.size());
    for (size_t vertexIndex = 0; vertexIndex < reversedPoints.size(); ++vertexIndex)
    {
        const size_t mirroredIndex = originalPoints.size() - 1 - vertexIndex;
        REQUIRE(reversedPoints[vertexIndex] == originalPoints[mirroredIndex]);
    }
}

void requireFaceUnchanged(const geo::Mesh& reversed, const geo::Mesh& original, Offset faceOffset)
{
    const std::span<const intT> reversedPoints = reversed.getFacePoints(faceOffset);
    const std::span<const intT> originalPoints = original.getFacePoints(faceOffset);
    REQUIRE(std::equal(
        reversedPoints.begin(), reversedPoints.end(), originalPoints.begin(), originalPoints.end()
    ));
}

} // namespace

TEST_CASE_METHOD(NMReset, "Every face is reversed when no selection is given")
{
    auto& nm = nt::nm();
    const GridAndReverse nodes = addReverseAfterGrid();
    nm.cook(nodes.reverse);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto reversed = getMesh(nm.getNode(nodes.reverse));

    REQUIRE(reversed->getNumFaces() == 2);
    requireFaceReversed(*reversed, *grid, 0);
    requireFaceReversed(*reversed, *grid, 1);
}

TEST_CASE_METHOD(NMReset, "Reversing a face flips its normal")
{
    auto& nm = nt::nm();
    const GridAndReverse nodes = addReverseAfterGrid();
    nm.cook(nodes.reverse);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto reversed = getMesh(nm.getNode(nodes.reverse));

    const Vector3 gridNormal = grid->getFaceNormal()[0];
    const Vector3 reversedNormal = reversed->getFaceNormal()[0];
    REQUIRE(reversedNormal.dot(gridNormal) == Catch::Approx(-1.0).margin(1e-5));
}

TEST_CASE_METHOD(NMReset, "A selection limits which faces are reversed")
{
    auto& nm = nt::nm();
    const GridAndReverse nodes = addReverseAfterGrid();
    nm.getNode(nodes.reverse).getParameter("selection").lock()->setString("f{1}");
    nm.cook(nodes.reverse);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto reversed = getMesh(nm.getNode(nodes.reverse));

    requireFaceUnchanged(*reversed, *grid, 0);
    requireFaceReversed(*reversed, *grid, 1);
}
