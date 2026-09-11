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

// Returns a cooked sphere's mesh.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Creates a sphere node without cooking it.
nt::NodeId addSphere()
{
    nt::NodeLoader::loadNodes();
    return nt::nm().createNode(nt::NodeTypeTable::requireNodeType("enzo::sphere"));
}

} // namespace

TEST_CASE_METHOD(NMReset, "The rows at the two ends come to a single pole point")
{
    auto& nm = nt::nm();
    const nt::NodeId sphere = addSphere();

    nm.getNode(sphere).getParameter("columns").lock()->setInt(8);
    nm.getNode(sphere).getParameter("rows").lock()->setInt(4);
    nm.cook(sphere);

    const auto mesh = getMesh(nm.getNode(sphere));

    // Three rings of eight, and a pole at each end.
    REQUIRE(mesh->getNumPoints() == 26);
    // A fan of eight triangles at each pole, with two bands of eight in between.
    REQUIRE(mesh->getNumFaces() == 32);
}

TEST_CASE_METHOD(NMReset, "The lowest row of a two row sphere joins both poles")
{
    auto& nm = nt::nm();
    const nt::NodeId sphere = addSphere();

    nm.getNode(sphere).getParameter("columns").lock()->setInt(8);
    nm.getNode(sphere).getParameter("rows").lock()->setInt(2);
    nm.cook(sphere);

    const auto mesh = getMesh(nm.getNode(sphere));

    // One ring of eight around the middle, and a pole at each end.
    REQUIRE(mesh->getNumPoints() == 10);
    REQUIRE(mesh->getNumFaces() == 16);
}

TEST_CASE_METHOD(NMReset, "An open arc leaves the rings unjoined")
{
    auto& nm = nt::nm();
    const nt::NodeId sphere = addSphere();

    nm.getNode(sphere).getParameter("columns").lock()->setInt(8);
    nm.getNode(sphere).getParameter("rows").lock()->setInt(4);
    nm.getNode(sphere).getParameter("arc").lock()->setString("open_arc");
    nm.getNode(sphere).getParameter("arcAngles").lock()->setFloat(180.f, 1);
    nm.cook(sphere);

    const auto mesh = getMesh(nm.getNode(sphere));

    // Nine rim points at each of the three rings, plus the two poles.
    REQUIRE(mesh->getNumPoints() == 29);
    REQUIRE(mesh->getNumFaces() == 32);
}

TEST_CASE_METHOD(NMReset, "A closed arc seals the sphere down each side")
{
    auto& nm = nt::nm();
    const nt::NodeId sphere = addSphere();

    nm.getNode(sphere).getParameter("columns").lock()->setInt(8);
    nm.getNode(sphere).getParameter("rows").lock()->setInt(4);
    nm.getNode(sphere).getParameter("arc").lock()->setString("closed_arc");
    nm.getNode(sphere).getParameter("arcAngles").lock()->setFloat(180.f, 1);
    nm.cook(sphere);

    const auto mesh = getMesh(nm.getNode(sphere));

    // Sealing the sides adds the point on the axis to each of the three rings.
    REQUIRE(mesh->getNumPoints() == 32);
    // The open arc's thirty two faces, plus a wall down each side of every row.
    REQUIRE(mesh->getNumFaces() == 40);
}

TEST_CASE_METHOD(NMReset, "The radius stretches each axis on its own")
{
    auto& nm = nt::nm();
    const nt::NodeId sphere = addSphere();

    nm.getNode(sphere).getParameter("radius").lock()->setFloat(3.f, 1);
    nm.cook(sphere);

    const auto mesh = getMesh(nm.getNode(sphere));

    // The bottom pole is the first point, and it sits at the far end of Y.
    REQUIRE(mesh->getPointPos(0).y() == Catch::Approx(-3.f).margin(1e-5));
}

TEST_CASE_METHOD(NMReset, "The axis parameter moves the poles")
{
    auto& nm = nt::nm();
    const nt::NodeId sphere = addSphere();

    nm.getNode(sphere).getParameter("axis").lock()->setString("x");
    nm.getNode(sphere).getParameter("radius").lock()->setFloat(2.f, 1);
    nm.cook(sphere);

    const auto mesh = getMesh(nm.getNode(sphere));

    REQUIRE(mesh->getPointPos(0).x() == Catch::Approx(-2.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(0).y() == Catch::Approx(0.f).margin(1e-5));
}
