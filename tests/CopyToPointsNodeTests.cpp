#include "Engine/Attribute/AttributeHandle.h"
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

// Returns the mesh a cooked node put on its first output.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Returns a single quad grid of four points.
nt::NodeId addQuadGrid(floatT size)
{
    auto& nm = nt::nm();
    const nt::NodeId grid = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nm.getNode(grid).getParameter("rows").lock()->setInt(2);
    nm.getNode(grid).getParameter("columns").lock()->setInt(2);
    nm.getNode(grid).getParameter("size").lock()->setFloat(size);
    return grid;
}

// Returns an attribute create node after the input, writing a float on points or a vector on faces.
nt::NodeId addAttributeCreate(nt::NodeId input, const std::string& name, const std::string& type)
{
    auto& nm = nt::nm();
    const nt::NodeId attributeCreate =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::attributeCreate"));
    nm.connectNodes(input, 0, attributeCreate, 0);

    auto& node = nm.getNode(attributeCreate);
    node.getParameter("name").lock()->setString(name);
    node.getParameter("type").lock()->setString(type);
    if (type == "float")
    {
        node.getParameter("floatValue").lock()->setFloat(2.5f);
    }
    else
    {
        node.getParameter("attachTo").lock()->setString("face");
        auto vectorValue = node.getParameter("vectorValue").lock();
        vectorValue->setFloat(1.f, 0);
        vectorValue->setFloat(2.f, 1);
        vectorValue->setFloat(3.f, 2);
    }
    return attributeCreate;
}

// The nodes of a quad copied onto the four corners of a larger quad.
struct QuadCopies
{
    nt::NodeId prototype;
    nt::NodeId targetPoints;
    nt::NodeId copy;
};

// Returns the cooked nodes of a quad carrying a float point attribute and a vector face attribute,
// copied onto the corners of a larger quad.
QuadCopies addQuadCopiedOntoQuad()
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    QuadCopies nodes;
    const nt::NodeId withFloat = addAttributeCreate(addQuadGrid(1.f), "weight", "float");
    nodes.prototype = addAttributeCreate(withFloat, "tint", "vector");
    nodes.targetPoints = addQuadGrid(10.f);
    nodes.copy = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::copyToPoints"));
    nm.connectNodes(nodes.prototype, 0, nodes.copy, 0);
    nm.connectNodes(nodes.targetPoints, 0, nodes.copy, 1);
    nm.cook(nodes.copy);
    return nodes;
}

void requirePointsMatch(const Vector3& point, const Vector3& expected)
{
    REQUIRE(point.x() == Catch::Approx(expected.x()).margin(1e-5));
    REQUIRE(point.y() == Catch::Approx(expected.y()).margin(1e-5));
    REQUIRE(point.z() == Catch::Approx(expected.z()).margin(1e-5));
}

} // namespace

TEST_CASE_METHOD(NMReset, "Each target point gets a copy moved onto it")
{
    auto& nm = nt::nm();
    const QuadCopies nodes = addQuadCopiedOntoQuad();

    const auto prototype = getMesh(nm.getNode(nodes.prototype));
    const auto targetPoints = getMesh(nm.getNode(nodes.targetPoints));
    const auto copies = getMesh(nm.getNode(nodes.copy));

    const Offset prototypePointCount = prototype->getNumPoints();
    REQUIRE(copies->getNumPoints() == prototypePointCount * targetPoints->getNumPoints());
    REQUIRE(copies->getNumFaces() == prototype->getNumFaces() * targetPoints->getNumPoints());

    for (Offset copyIndex = 0; copyIndex < targetPoints->getNumPoints(); ++copyIndex)
    {
        for (Offset pointIndex = 0; pointIndex < prototypePointCount; ++pointIndex)
        {
            const Offset copiedPoint = copyIndex * prototypePointCount + pointIndex;
            const Vector3 expected =
                targetPoints->getPointPos(copyIndex) + prototype->getPointPos(pointIndex);
            requirePointsMatch(copies->getPointPos(copiedPoint), expected);
        }
    }
}

TEST_CASE_METHOD(NMReset, "Each copy's faces use the points of that copy")
{
    auto& nm = nt::nm();
    const QuadCopies nodes = addQuadCopiedOntoQuad();

    const auto prototype = getMesh(nm.getNode(nodes.prototype));
    const auto copies = getMesh(nm.getNode(nodes.copy));

    const Offset prototypePointCount = prototype->getNumPoints();
    const Offset prototypeFaceCount = prototype->getNumFaces();
    for (Offset faceOffset = 0; faceOffset < copies->getNumFaces(); ++faceOffset)
    {
        const Offset copyIndex = faceOffset / prototypeFaceCount;
        const Offset prototypeFace = faceOffset % prototypeFaceCount;
        const auto copiedPoints = copies->getFacePoints(faceOffset);
        const auto prototypePoints = prototype->getFacePoints(prototypeFace);
        REQUIRE(copiedPoints.size() == prototypePoints.size());
        for (size_t vertexIndex = 0; vertexIndex < copiedPoints.size(); ++vertexIndex)
        {
            const Offset expected = copyIndex * prototypePointCount + prototypePoints[vertexIndex];
            REQUIRE(copiedPoints[vertexIndex] == expected);
        }
    }
}

TEST_CASE_METHOD(NMReset, "Copies carry the prototype's attributes")
{
    auto& nm = nt::nm();
    const QuadCopies nodes = addQuadCopiedOntoQuad();
    const auto copies = getMesh(nm.getNode(nodes.copy));

    const auto weightAttribute = copies->getAttribByName(attr::AttributeOwner::POINT, "weight");
    REQUIRE(weightAttribute != nullptr);
    const attr::AttributeHandleRO<floatT> weight(weightAttribute);
    REQUIRE(weight.getSize() == copies->getNumPoints());
    for (const floatT value : weight.getAllValues())
        REQUIRE(value == Catch::Approx(2.5f));

    const auto tintAttribute = copies->getAttribByName(attr::AttributeOwner::FACE, "tint");
    REQUIRE(tintAttribute != nullptr);
    const attr::AttributeHandleRO<Vector3> tint(tintAttribute);
    REQUIRE(tint.getSize() == copies->getNumFaces());
    for (const Vector3& value : tint.getAllValues())
        REQUIRE(value == Vector3(1.f, 2.f, 3.f));
}
