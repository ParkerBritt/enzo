#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>

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

// A grid feeding a script node that runs the given code, cooked when built.
struct ScriptGraph
{
    nt::NodeId grid = 0;
    nt::NodeId script = 0;

    explicit ScriptGraph(const String& code)
    {
        auto& nm = nt::nm();
        grid = nm.createNode("enzo::grid");
        script = nm.createNode("enzo::script");
        nm.connectNodes(grid, 0, script, 0);
        nm.getNode(script).getParameter("code").lock()->setString(code);
        nm.cook(script);
    }

    std::shared_ptr<const geo::Mesh> getOutput() const
    {
        return getMesh(nt::nm().getNode(script));
    }
};

// Returns a point attribute of the mesh, failing the test when it is missing.
template <typename Value>
attr::AttributeHandleRO<Value> getPointAttribute(const geo::Mesh& mesh, const String& name)
{
    const auto attribute = mesh.getAttribByName(attr::AttributeOwner::POINT, name, true);
    REQUIRE(attribute != nullptr);
    return attr::AttributeHandleRO<Value>(attribute);
}

} // namespace

// Writing attributes

TEST_CASE_METHOD(NMReset, "A script writes a new attribute on every point")
{
    const ScriptGraph graph("@height = 5.0");
    const auto mesh = graph.getOutput();

    const auto height = getPointAttribute<floatT>(*mesh, "height");
    REQUIRE(mesh->getNumPoints() > 0);
    for (Offset pointOffset = 0; pointOffset < mesh->getNumPoints(); ++pointOffset)
        REQUIRE(height[pointOffset] == Catch::Approx(5.0));
}

TEST_CASE_METHOD(NMReset, "A script writes a different value for each point")
{
    const ScriptGraph graph("i@id = curPt() * 2");
    const auto mesh = graph.getOutput();

    const auto id = getPointAttribute<intT>(*mesh, "id");
    for (Offset pointOffset = 0; pointOffset < mesh->getNumPoints(); ++pointOffset)
        REQUIRE(id[pointOffset] == intT(pointOffset) * 2);
}

TEST_CASE_METHOD(NMReset, "A script edits the positions it reads")
{
    auto& nm = nt::nm();
    const ScriptGraph graph("@Position.y += 1.0");

    const auto inputMesh = getMesh(nm.getNode(graph.grid));
    const auto outputMesh = graph.getOutput();

    for (Offset pointOffset = 0; pointOffset < inputMesh->getNumPoints(); ++pointOffset)
    {
        const Vector3 before = inputMesh->getPointPos(pointOffset);
        const Vector3 after = outputMesh->getPointPos(pointOffset);
        REQUIRE(after.y() == Catch::Approx(before.y() + 1.0));
        REQUIRE(after.x() == Catch::Approx(before.x()));
    }
}

TEST_CASE_METHOD(NMReset, "An empty script leaves the geometry alone")
{
    auto& nm = nt::nm();
    const ScriptGraph graph("");

    REQUIRE(graph.getOutput()->getNumPoints() == getMesh(nm.getNode(graph.grid))->getNumPoints());
}

// Running the same result in parallel

TEST_CASE_METHOD(NMReset, "A script over many points matches a serial run")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = nm.createNode("enzo::grid");
    nm.getNode(grid).getParameter("rows").lock()->setInt(200);
    nm.getNode(grid).getParameter("columns").lock()->setInt(200);

    const nt::NodeId script = nm.createNode("enzo::script");
    nm.connectNodes(grid, 0, script, 0);
    nm.getNode(script).getParameter("code").lock()->setString("@height = float(curPt()) * 0.5");
    nm.cook(script);

    const auto mesh = getMesh(nm.getNode(script));
    REQUIRE(mesh->getNumPoints() > 1024);

    const auto height = getPointAttribute<floatT>(*mesh, "height");
    for (Offset pointOffset = 0; pointOffset < mesh->getNumPoints(); ++pointOffset)
        REQUIRE(height[pointOffset] == Catch::Approx(floatT(pointOffset) * 0.5));
}

TEST_CASE_METHOD(NMReset, "A script numbers the points left after a delete in order")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = nm.createNode("enzo::grid");
    nm.getNode(grid).getParameter("rows").lock()->setInt(200);
    nm.getNode(grid).getParameter("columns").lock()->setInt(200);

    const nt::NodeId deleteNode = nm.createNode("enzo::delete");
    nm.connectNodes(grid, 0, deleteNode, 0);
    nm.getNode(deleteNode).getParameter("selection").lock()->setString("p{0}");

    const nt::NodeId script = nm.createNode("enzo::script");
    nm.connectNodes(deleteNode, 0, script, 0);
    nm.getNode(script).getParameter("code").lock()->setString(R"(i@index = curPt()
i@count = ptCount())");
    nm.cook(script);

    const auto mesh = getMesh(nm.getNode(script));
    const auto index = getPointAttribute<intT>(*mesh, "index");
    const auto count = getPointAttribute<intT>(*mesh, "count");

    intT expectedIndex = 0;
    for (Offset pointOffset = 0; pointOffset < mesh->getNumPoints(); ++pointOffset)
    {
        if (!mesh->isValidPoint(pointOffset)) continue;
        REQUIRE(index[pointOffset] == expectedIndex);
        ++expectedIndex;
    }
    REQUIRE(expectedIndex > 1024);
    REQUIRE(count[mesh->getNumPoints() - 1] == expectedIndex);
}

// Recooking

TEST_CASE_METHOD(NMReset, "A script reading a parameter recooks when that parameter changes")
{
    auto& nm = nt::nm();
    const nt::NodeId transform = nm.createNode("enzo::transform");
    const std::shared_ptr<prm::NodeParameter> translate =
        nm.getNode(transform).getParameter("translate").lock();
    translate->setFloat(2.0f, 1);

    const ScriptGraph graph("@height = prm(\"transform1.translate\", 1)");
    REQUIRE(getPointAttribute<floatT>(*graph.getOutput(), "height")[0] == Catch::Approx(2.0));

    translate->setFloat(3.0f, 1);
    nm.cook(graph.script);

    REQUIRE(getPointAttribute<floatT>(*graph.getOutput(), "height")[0] == Catch::Approx(3.0));
}

TEST_CASE_METHOD(NMReset, "A script reading the frame recooks when the frame changes")
{
    auto& nm = nt::nm();
    nm.setFrame(1.0f);
    const ScriptGraph graph("@height = frame()");
    REQUIRE(getPointAttribute<floatT>(*graph.getOutput(), "height")[0] == Catch::Approx(1.0));

    nm.setFrame(7.0f);
    nm.cook(graph.script);

    REQUIRE(getPointAttribute<floatT>(*graph.getOutput(), "height")[0] == Catch::Approx(7.0));
}

// Errors

TEST_CASE_METHOD(NMReset, "A script that fails to compile leaves the geometry alone")
{
    auto& nm = nt::nm();
    const ScriptGraph graph("@height = ");

    REQUIRE(nm.getNode(graph.script).getOutputPacket(0)->size() == 0);
}

// Reading the input

TEST_CASE_METHOD(NMReset, "A script reads its input while writing the same attribute")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = nm.createNode("enzo::grid");
    nm.getNode(grid).getParameter("rows").lock()->setInt(200);
    nm.getNode(grid).getParameter("columns").lock()->setInt(200);

    const nt::NodeId script = nm.createNode("enzo::script");
    nm.connectNodes(grid, 0, script, 0);
    nm.getNode(script).getParameter("code").lock()->setString(
        R"(@Position = pointAttrVector("Position", (curPt() + 1l) % ptCount()))"
    );
    nm.cook(script);

    const auto input = getMesh(nm.getNode(grid));
    const auto output = getMesh(nm.getNode(script));
    const Offset pointCount = input->getNumPoints();
    REQUIRE(pointCount > 1024);
    for (Offset pointOffset = 0; pointOffset < pointCount; ++pointOffset)
    {
        const Offset nextOffset = (pointOffset + 1) % pointCount;
        REQUIRE(output->getPointPos(pointOffset) == input->getPointPos(nextOffset));
    }
}
