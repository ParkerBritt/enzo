#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// Returns the first point of a cooked node's output.
Vector3 getFirstPoint(nt::Node& node)
{
    const auto mesh = std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->getNumPoints() > 0);
    return mesh->getPointPos(0);
}

} // namespace

TEST_CASE_METHOD(NMReset, "Rotate is read in degrees")
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    // A grid feeding a transform that turns a quarter circle about Y
    const nt::NodeId grid = nm.createNode(nt::NodeTypeTable::requireNodeType("grid"));
    const nt::NodeId transform = nm.createNode(nt::NodeTypeTable::requireNodeType("transform"));
    nm.connectNodes(grid, 0, transform, 0);

    nm.getNode(transform).getParameter("rotate").lock()->setFloat(90.f, 1);
    nm.cook(transform);

    const Vector3 before = getFirstPoint(nm.getNode(grid));
    const Vector3 after = getFirstPoint(nm.getNode(transform));

    // Turning about Y sends the X axis onto the negative Z axis
    REQUIRE(after.x() == Catch::Approx(before.z()).margin(1e-5));
    REQUIRE(after.y() == Catch::Approx(before.y()).margin(1e-5));
    REQUIRE(after.z() == Catch::Approx(-before.x()).margin(1e-5));
}
