#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
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

// Returns a cooked spiral's mesh.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Creates a spiral node without cooking it.
nt::NodeId addSpiral()
{
    nt::NodeLoader::loadNodes();
    return nt::nm().createNode("enzo::spiral");
}

} // namespace

TEST_CASE_METHOD(NMReset, "A spiral is one open face with points spread over its turns")
{
    auto& nm = nt::nm();
    const nt::NodeId spiral = addSpiral();

    nm.getNode(spiral).getParameter("turns").lock()->setFloat(2.f);
    nm.getNode(spiral).getParameter("pointsPerTurn").lock()->setInt(8);
    nm.cook(spiral);

    const auto mesh = getMesh(nm.getNode(spiral));

    // Expects 2 * 8 = 16 segments, which need 17 points.
    REQUIRE(mesh->getNumPoints() == 17);
    REQUIRE(mesh->getNumFaces() == 1);
    REQUIRE_FALSE(mesh->isClosed(0));
}

TEST_CASE_METHOD(NMReset, "The spiral rises from the start radius to the end radius")
{
    auto& nm = nt::nm();
    const nt::NodeId spiral = addSpiral();

    nm.getNode(spiral).getParameter("radius").lock()->setFloat(1.f, 0);
    nm.getNode(spiral).getParameter("radius").lock()->setFloat(3.f, 1);
    nm.getNode(spiral).getParameter("height").lock()->setFloat(4.f);
    nm.getNode(spiral).getParameter("turns").lock()->setFloat(2.f);
    nm.cook(spiral);

    const auto mesh = getMesh(nm.getNode(spiral));
    const Offset lastPoint = mesh->getNumPoints() - 1;

    // Expects both ends on the Z axis, since 2 turns end at the starting angle.
    REQUIRE(mesh->getPointPos(0).y() == Catch::Approx(-2.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(0).z() == Catch::Approx(1.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(lastPoint).y() == Catch::Approx(2.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(lastPoint).z() == Catch::Approx(3.f).margin(1e-4));
}

TEST_CASE_METHOD(NMReset, "A clockwise spiral turns the other way")
{
    auto& nm = nt::nm();
    const nt::NodeId spiral = addSpiral();

    nm.getNode(spiral).getParameter("turns").lock()->setFloat(1.f);
    nm.getNode(spiral).getParameter("pointsPerTurn").lock()->setInt(4);
    nm.cook(spiral);
    const float counterclockwiseX = getMesh(nm.getNode(spiral))->getPointPos(1).x();

    nm.getNode(spiral).getParameter("turnDirection").lock()->setString("clockwise");
    nm.cook(spiral);
    const float clockwiseX = getMesh(nm.getNode(spiral))->getPointPos(1).x();

    REQUIRE(counterclockwiseX == Catch::Approx(1.f).margin(1e-5));
    REQUIRE(clockwiseX == Catch::Approx(-1.f).margin(1e-5));
}
