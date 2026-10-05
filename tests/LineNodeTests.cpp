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

// Returns a cooked line's mesh.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Creates a line node without cooking it.
nt::NodeId addLine()
{
    nt::NodeLoader::loadNodes();
    return nt::nm().createNode("enzo::line");
}

} // namespace

TEST_CASE_METHOD(NMReset, "A line is one open face through all its points")
{
    auto& nm = nt::nm();
    const nt::NodeId line = addLine();

    nm.getNode(line).getParameter("points").lock()->setInt(5);
    nm.cook(line);

    const auto mesh = getMesh(nm.getNode(line));

    REQUIRE(mesh->getNumPoints() == 5);
    REQUIRE(mesh->getNumFaces() == 1);
    REQUIRE(mesh->getFacePointCount(0) == 5);
    REQUIRE_FALSE(mesh->isClosed(0));
}

TEST_CASE_METHOD(NMReset, "The points are spread evenly from the origin")
{
    auto& nm = nt::nm();
    const nt::NodeId line = addLine();

    nm.getNode(line).getParameter("origin").lock()->setFloat(1.f, 0);
    nm.getNode(line).getParameter("length").lock()->setFloat(4.f);
    nm.getNode(line).getParameter("points").lock()->setInt(3);
    nm.cook(line);

    const auto mesh = getMesh(nm.getNode(line));

    REQUIRE(mesh->getPointPos(0).x() == Catch::Approx(1.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(1).y() == Catch::Approx(2.f).margin(1e-5));
    REQUIRE(mesh->getPointPos(2).y() == Catch::Approx(4.f).margin(1e-5));
}

TEST_CASE_METHOD(NMReset, "The length of the direction does not change the line")
{
    auto& nm = nt::nm();
    const nt::NodeId line = addLine();

    nm.getNode(line).getParameter("direction").lock()->setFloat(0.f, 1);
    nm.getNode(line).getParameter("direction").lock()->setFloat(3.f, 2);
    nm.getNode(line).getParameter("length").lock()->setFloat(2.f);
    nm.cook(line);

    const auto mesh = getMesh(nm.getNode(line));

    REQUIRE(mesh->getPointPos(1).z() == Catch::Approx(2.f).margin(1e-5));
}
