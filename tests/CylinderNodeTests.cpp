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

// Returns a cooked cylinder's mesh.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Creates a cylinder node without cooking it.
nt::NodeId addCylinder()
{
    nt::NodeLoader::loadNodes();
    return nt::nm().createNode(nt::NodeTypeTable::requireNodeType("enzo::cylinder"));
}

} // namespace

TEST_CASE_METHOD(NMReset, "A closed cylinder has one ring of points per row boundary")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("columns").lock()->setInt(8);
    nm.getNode(cylinder).getParameter("rows").lock()->setInt(3);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // Four rings of eight, and the welded caps add none of their own.
    REQUIRE(mesh->getNumPoints() == 32);
    // Three bands of eight side faces, plus a cap at each end.
    REQUIRE(mesh->getNumFaces() == 26);
}

TEST_CASE_METHOD(NMReset, "Unwelded caps get their own copy of the rim")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("columns").lock()->setInt(8);
    nm.getNode(cylinder).getParameter("weldCaps").lock()->setInt(0);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // Two rings for the side and one more for each cap.
    REQUIRE(mesh->getNumPoints() == 32);
}

TEST_CASE_METHOD(NMReset, "A radius of zero drops the cap at that end")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("columns").lock()->setInt(8);
    nm.getNode(cylinder).getParameter("radius").lock()->setFloat(0.f, 0);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // Eight side faces and only the bottom cap.
    REQUIRE(mesh->getNumFaces() == 9);
}

TEST_CASE_METHOD(NMReset, "An open arc leaves the ring unjoined")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("columns").lock()->setInt(8);
    nm.getNode(cylinder).getParameter("arc").lock()->setString("open_arc");
    nm.getNode(cylinder).getParameter("arcAngles").lock()->setFloat(180.f, 1);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // Nine rim points at each of the two rings, with nothing on the axis.
    REQUIRE(mesh->getNumPoints() == 18);
    // Eight side faces and a cap at each end.
    REQUIRE(mesh->getNumFaces() == 10);
}

TEST_CASE_METHOD(NMReset, "A closed arc seals its open sides")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("columns").lock()->setInt(8);
    nm.getNode(cylinder).getParameter("arc").lock()->setString("closed_arc");
    nm.getNode(cylinder).getParameter("arcAngles").lock()->setFloat(180.f, 1);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // The open arc's ten faces, plus a wall down each side.
    REQUIRE(mesh->getNumFaces() == 12);
    // Sealing the sides adds the point on the axis to each ring.
    REQUIRE(mesh->getNumPoints() == 20);
}

TEST_CASE_METHOD(NMReset, "Uniform radius multiplies both ends")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("radius").lock()->setFloat(2.f, 0);
    nm.getNode(cylinder).getParameter("radius").lock()->setFloat(1.f, 1);
    nm.getNode(cylinder).getParameter("uniformRadius").lock()->setFloat(3.f);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // The first ring is the bottom one, so its radius is 1 times 3.
    REQUIRE(mesh->getPointPos(0).z() == Catch::Approx(3.f).margin(1e-5));

    // The rings are built bottom first, so the top one starts at the halfway point.
    const Offset topRingStart = mesh->getNumPoints() / 2;
    REQUIRE(mesh->getPointPos(topRingStart).z() == Catch::Approx(6.f).margin(1e-5));
}

TEST_CASE_METHOD(NMReset, "The axis parameter stands the cylinder up")
{
    auto& nm = nt::nm();
    const nt::NodeId cylinder = addCylinder();

    nm.getNode(cylinder).getParameter("axis").lock()->setString("x");
    nm.getNode(cylinder).getParameter("height").lock()->setFloat(4.f);
    nm.cook(cylinder);

    const auto mesh = getMesh(nm.getNode(cylinder));

    // The first ring sits at the bottom of the cylinder, which along X is at -2.
    REQUIRE(mesh->getPointPos(0).x() == Catch::Approx(-2.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(0).y() == Catch::Approx(0.f).margin(1e-5));
}
