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
#include <string_view>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// A grid feeding a transform node.
struct GridAndTransform
{
    nt::NodeId grid;
    nt::NodeId transform;
};

// Builds a grid wired into a transform node, without cooking either.
GridAndTransform addTransformAfterGrid()
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    GridAndTransform nodes;
    nodes.grid = nm.createNode("enzo::grid");
    nodes.transform = nm.createNode("enzo::transform");
    nm.connectNodes(nodes.grid, 0, nodes.transform, 0);

    return nodes;
}

// Sets all three components of a vector parameter.
void setVectorParameter(nt::Node& node, std::string_view parameterName, const Vector3& value)
{
    auto parameter = node.getParameter(parameterName).lock();
    parameter->setFloat(value.x(), 0);
    parameter->setFloat(value.y(), 1);
    parameter->setFloat(value.z(), 2);
}

// Returns one point of a cooked node's output.
Vector3 getPoint(nt::Node& node, Offset pointOffset)
{
    const auto mesh = std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->getNumPoints() > pointOffset);
    return mesh->getPointPosition(pointOffset);
}

// Returns the first point of a cooked node's output.
Vector3 getFirstPoint(nt::Node& node) { return getPoint(node, 0); }

void requirePointsMatch(const Vector3& point, const Vector3& expected)
{
    REQUIRE(point.x() == Catch::Approx(expected.x()).margin(1e-5));
    REQUIRE(point.y() == Catch::Approx(expected.y()).margin(1e-5));
    REQUIRE(point.z() == Catch::Approx(expected.z()).margin(1e-5));
}

} // namespace

TEST_CASE_METHOD(NMReset, "Rotate is read in degrees")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();

    // Turn a quarter circle about Y
    nm.getNode(nodes.transform).getParameter("rotate").lock()->setFloat(90.f, 1);
    nm.cook(nodes.transform);

    const Vector3 before = getFirstPoint(nm.getNode(nodes.grid));
    const Vector3 after = getFirstPoint(nm.getNode(nodes.transform));

    // Turning about Y sends the X axis onto the negative Z axis
    requirePointsMatch(after, Vector3(before.z(), before.y(), -before.x()));
}

TEST_CASE_METHOD(NMReset, "Scale multiplies each axis independently")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();

    setVectorParameter(nm.getNode(nodes.transform), "scale", Vector3(2, 3, 4));
    nm.cook(nodes.transform);

    const Vector3 before = getFirstPoint(nm.getNode(nodes.grid));
    const Vector3 after = getFirstPoint(nm.getNode(nodes.transform));

    requirePointsMatch(after, before.cwiseProduct(Vector3(2, 3, 4)));
}

TEST_CASE_METHOD(NMReset, "A point selection leaves the other points where they are")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();

    // Lift only the first point of the grid
    nm.getNode(nodes.transform).getParameter("selection").lock()->setString("p{0}");
    nm.getNode(nodes.transform).getParameter("translate").lock()->setFloat(5.f, 1);
    nm.cook(nodes.transform);

    const Vector3 firstBefore = getPoint(nm.getNode(nodes.grid), 0);
    const Vector3 firstAfter = getPoint(nm.getNode(nodes.transform), 0);
    requirePointsMatch(firstAfter, firstBefore + Vector3(0, 5, 0));

    const Vector3 secondBefore = getPoint(nm.getNode(nodes.grid), 1);
    const Vector3 secondAfter = getPoint(nm.getNode(nodes.transform), 1);
    requirePointsMatch(secondAfter, secondBefore);
}

TEST_CASE_METHOD(NMReset, "Rotate turns about the pivot")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();
    nm.cook(nodes.grid);

    // Place the pivot on the first point so it stays put while the rest turns
    const Vector3 firstBefore = getPoint(nm.getNode(nodes.grid), 0);
    setVectorParameter(nm.getNode(nodes.transform), "pivot", firstBefore);
    nm.getNode(nodes.transform).getParameter("rotate").lock()->setFloat(90.f, 1);
    nm.cook(nodes.transform);

    const Vector3 firstAfter = getPoint(nm.getNode(nodes.transform), 0);
    requirePointsMatch(firstAfter, firstBefore);

    // Turning about Y sends the offset from the pivot along X onto negative Z
    const Vector3 secondBefore = getPoint(nm.getNode(nodes.grid), 1);
    const Vector3 secondAfter = getPoint(nm.getNode(nodes.transform), 1);
    const Vector3 offsetBefore = secondBefore - firstBefore;
    const Vector3 offsetAfter = secondAfter - firstBefore;
    requirePointsMatch(offsetAfter, Vector3(offsetBefore.z(), offsetBefore.y(), -offsetBefore.x()));
}

TEST_CASE_METHOD(NMReset, "Scale grows away from the pivot")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();
    nm.cook(nodes.grid);

    const Vector3 firstBefore = getPoint(nm.getNode(nodes.grid), 0);
    setVectorParameter(nm.getNode(nodes.transform), "pivot", firstBefore);
    nm.getNode(nodes.transform).getParameter("uniform_scale").lock()->setFloat(2.f);
    nm.cook(nodes.transform);

    const Vector3 secondBefore = getPoint(nm.getNode(nodes.grid), 1);
    const Vector3 secondAfter = getPoint(nm.getNode(nodes.transform), 1);
    requirePointsMatch(secondAfter, firstBefore + (secondBefore - firstBefore) * 2.f);
}

TEST_CASE_METHOD(NMReset, "Pivot Rotate turns the axes that scale works along")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();

    // Turn the pivot a quarter circle about Y so its X axis lies along world Z
    nm.getNode(nodes.transform).getParameter("pivot_rotate").lock()->setFloat(90.f, 1);
    nm.getNode(nodes.transform).getParameter("scale").lock()->setFloat(2.f, 0);
    nm.cook(nodes.transform);

    const Vector3 before = getFirstPoint(nm.getNode(nodes.grid));
    const Vector3 after = getFirstPoint(nm.getNode(nodes.transform));
    requirePointsMatch(after, Vector3(before.x(), before.y(), before.z() * 2.f));
}

TEST_CASE_METHOD(NMReset, "Pivot Rotate leaves the translate direction alone")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();

    nm.getNode(nodes.transform).getParameter("pivot_rotate").lock()->setFloat(90.f, 1);
    nm.getNode(nodes.transform).getParameter("translate").lock()->setFloat(5.f, 0);
    nm.cook(nodes.transform);

    const Vector3 before = getFirstPoint(nm.getNode(nodes.grid));
    const Vector3 after = getFirstPoint(nm.getNode(nodes.transform));
    requirePointsMatch(after, before + Vector3(5, 0, 0));
}

TEST_CASE_METHOD(NMReset, "Translate Rotate Scale swings the moved points around the pivot")
{
    auto& nm = nt::nm();
    const GridAndTransform nodes = addTransformAfterGrid();

    nm.getNode(nodes.transform).getParameter("transform_order").lock()->setString("trs");
    nm.getNode(nodes.transform).getParameter("translate").lock()->setFloat(5.f, 0);
    nm.getNode(nodes.transform).getParameter("rotate").lock()->setFloat(90.f, 1);
    nm.cook(nodes.transform);

    // Turning about Y sends the moved point's X axis onto the negative Z axis
    const Vector3 before = getFirstPoint(nm.getNode(nodes.grid));
    const Vector3 moved = before + Vector3(5, 0, 0);
    const Vector3 after = getFirstPoint(nm.getNode(nodes.transform));
    requirePointsMatch(after, Vector3(moved.z(), moved.y(), -moved.x()));
}
