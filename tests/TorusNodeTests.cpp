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

// Returns a cooked torus's mesh.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Creates a torus node without cooking it.
nt::NodeId addTorus()
{
    nt::NodeLoader::loadNodes();
    return nt::nm().createNode("enzo::torus");
}

} // namespace

TEST_CASE_METHOD(NMReset, "A torus has one point per column and row")
{
    auto& nm = nt::nm();
    const nt::NodeId torus = addTorus();

    nm.getNode(torus).getParameter("columns").lock()->setInt(8);
    nm.getNode(torus).getParameter("rows").lock()->setInt(4);
    nm.cook(torus);

    const auto mesh = getMesh(nm.getNode(torus));

    // Expects 8 * 4 points and faces, since the surface wraps round both ways.
    REQUIRE(mesh->getNumPoints() == 32);
    REQUIRE(mesh->getNumFaces() == 32);
}

TEST_CASE_METHOD(NMReset, "The first point sits on the outer edge of the torus")
{
    auto& nm = nt::nm();
    const nt::NodeId torus = addTorus();

    nm.getNode(torus).getParameter("radius").lock()->setFloat(2.f, 0);
    nm.getNode(torus).getParameter("radius").lock()->setFloat(0.5f, 1);
    nm.cook(torus);

    const auto mesh = getMesh(nm.getNode(torus));

    REQUIRE(mesh->getPointPos(0).x() == Catch::Approx(0.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(0).y() == Catch::Approx(0.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(0).z() == Catch::Approx(2.5f).margin(1e-5));
}

TEST_CASE_METHOD(NMReset, "The axis parameter turns the hole of the torus")
{
    auto& nm = nt::nm();
    const nt::NodeId torus = addTorus();

    nm.getNode(torus).getParameter("axis").lock()->setString("z");
    nm.cook(torus);

    const auto mesh = getMesh(nm.getNode(torus));

    // Expects the first point on -Y, since putting the hole on Z turns +Z onto -Y.
    REQUIRE(mesh->getPointPos(0).y() == Catch::Approx(-1.5f).margin(1e-5));
    REQUIRE(mesh->getPointPos(0).z() == Catch::Approx(0.f).margin(1e-5));
}
