#include "Engine/Attribute/AttributeNames.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/Normals.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset()
    {
        nt::nm()._reset();
        nt::NodeLoader::loadNodes();
    }
    ~NMReset() { nt::nm()._reset(); }
};

// Returns the mesh a cooked node put on its first output.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

} // namespace

TEST_CASE_METHOD(NMReset, "A mountain is an attribute noise set to fractal noise along the normals")
{
    auto& nm = nt::nm();
    nt::Node& node = nm.getNode(nm.createNode("enzo::mountain"));

    REQUIRE(node.getType().getFullName() == "enzo::attributeNoise");
    REQUIRE(node.getPath() == "/attributeNoise1");
    REQUIRE(node.getParameter("name").lock()->evalString() == attr::names::position);
    REQUIRE(node.getParameter("type").lock()->evalString() == "vector");
    REQUIRE(node.getParameter("fractalType").lock()->evalString() == "fbm");
    REQUIRE(node.getParameter("alongVector").lock()->evalInt() == 1);
    REQUIRE(node.getParameter("alongVectorAttribute").lock()->evalString() == attr::names::normal);
}

TEST_CASE_METHOD(NMReset, "A mountain moves every point along its normal")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = nm.createNode("enzo::grid");
    const nt::NodeId mountain = nm.createNode("enzo::mountain");
    nm.connectNodes(grid, 0, mountain, 0);
    nm.getNode(mountain).getParameter("frequency").lock()->setFloat(0.37f);
    nm.cook(mountain);

    const auto inputMesh = getMesh(nm.getNode(grid));
    const auto outputMesh = getMesh(nm.getNode(mountain));
    REQUIRE(outputMesh->getNumPoints() == inputMesh->getNumPoints());
    const std::vector<Vector3> normals = utils::computePointNormals(*inputMesh);

    bool anyPointMoved = false;
    for (Offset pointOffset = 0; pointOffset < inputMesh->getNumPoints(); ++pointOffset)
    {
        const Vector3 movement =
            outputMesh->getPointPos(pointOffset) - inputMesh->getPointPos(pointOffset);
        REQUIRE(movement.cross(normals[pointOffset]).norm() == Catch::Approx(0.0f).margin(1e-4));
        if (!movement.isZero()) anyPointMoved = true;
    }
    REQUIRE(anyPointMoved);
}
