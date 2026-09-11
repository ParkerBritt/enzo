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

// A grid feeding a duplicate node.
struct GridAndDuplicate
{
    nt::NodeId grid;
    nt::NodeId duplicate;
};

// Returns the mesh a cooked node put on its first output.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Builds a single quad grid wired into a duplicate node, without cooking either.
GridAndDuplicate addDuplicateAfterGrid()
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    GridAndDuplicate nodes;
    nodes.grid = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nodes.duplicate = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::duplicate"));
    nm.connectNodes(nodes.grid, 0, nodes.duplicate, 0);

    nm.getNode(nodes.grid).getParameter("rows").lock()->setInt(1);
    nm.getNode(nodes.grid).getParameter("columns").lock()->setInt(1);

    return nodes;
}

void requirePointsMatch(const Vector3& point, const Vector3& expected)
{
    REQUIRE(point.x() == Catch::Approx(expected.x()).margin(1e-5));
    REQUIRE(point.y() == Catch::Approx(expected.y()).margin(1e-5));
    REQUIRE(point.z() == Catch::Approx(expected.z()).margin(1e-5));
}

} // namespace

TEST_CASE_METHOD(NMReset, "A count of one passes the geometry through unchanged")
{
    auto& nm = nt::nm();
    const GridAndDuplicate nodes = addDuplicateAfterGrid();

    nm.getNode(nodes.duplicate).getParameter("count").lock()->setInt(1);
    nm.getNode(nodes.duplicate).getParameter("translate").lock()->setFloat(10.f, 0);
    nm.cook(nodes.duplicate);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto duplicated = getMesh(nm.getNode(nodes.duplicate));

    REQUIRE(duplicated->getNumPoints() == grid->getNumPoints());
    requirePointsMatch(duplicated->getPointPos(0), grid->getPointPos(0));
}

TEST_CASE_METHOD(NMReset, "Each copy moves by the translate times its copy number")
{
    auto& nm = nt::nm();
    const GridAndDuplicate nodes = addDuplicateAfterGrid();

    nm.getNode(nodes.duplicate).getParameter("count").lock()->setInt(3);
    nm.getNode(nodes.duplicate).getParameter("translate").lock()->setFloat(10.f, 0);
    nm.cook(nodes.duplicate);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto duplicated = getMesh(nm.getNode(nodes.duplicate));

    const Offset pointsPerCopy = grid->getNumPoints();
    REQUIRE(duplicated->getNumPoints() == pointsPerCopy * 3);

    const Vector3 first = grid->getPointPos(0);
    requirePointsMatch(duplicated->getPointPos(0), first);
    requirePointsMatch(duplicated->getPointPos(pointsPerCopy), first + Vector3(10, 0, 0));
    requirePointsMatch(duplicated->getPointPos(pointsPerCopy * 2), first + Vector3(20, 0, 0));
}

TEST_CASE_METHOD(NMReset, "Each copy scales on top of the copy before it")
{
    auto& nm = nt::nm();
    const GridAndDuplicate nodes = addDuplicateAfterGrid();

    nm.getNode(nodes.duplicate).getParameter("count").lock()->setInt(3);
    nm.getNode(nodes.duplicate).getParameter("uniform_scale").lock()->setFloat(2.f);
    nm.cook(nodes.duplicate);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto duplicated = getMesh(nm.getNode(nodes.duplicate));

    const Offset pointsPerCopy = grid->getNumPoints();
    const Vector3 first = grid->getPointPos(0);
    requirePointsMatch(duplicated->getPointPos(pointsPerCopy), first * 2.f);
    requirePointsMatch(duplicated->getPointPos(pointsPerCopy * 2), first * 4.f);
}

TEST_CASE_METHOD(NMReset, "A primitive outside the selection is not duplicated")
{
    auto& nm = nt::nm();
    const GridAndDuplicate nodes = addDuplicateAfterGrid();

    nm.getNode(nodes.duplicate).getParameter("count").lock()->setInt(3);
    nm.getNode(nodes.duplicate).getParameter("selection").lock()->setString("/nothing");
    nm.cook(nodes.duplicate);

    const auto grid = getMesh(nm.getNode(nodes.grid));
    const auto duplicated = getMesh(nm.getNode(nodes.duplicate));

    REQUIRE(duplicated->getNumPoints() == grid->getNumPoints());
}
