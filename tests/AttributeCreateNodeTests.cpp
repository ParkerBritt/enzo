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

// Returns a read only view of the attribute the node wrote.
template <typename T>
attr::AttributeHandleRO<T> getAttribute(nt::Node& node, attr::AttributeOwner owner)
{
    const auto attribute = getMesh(node)->getAttribByName(owner, "test");
    REQUIRE(attribute != nullptr);
    return attr::AttributeHandleRO<T>(attribute);
}

// Returns an uncooked attribute create wired to a grid, writing under the name "test".
nt::NodeId addAttributeCreateAfterGrid()
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    const nt::NodeId grid = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    const nt::NodeId attributeCreate =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::attributeCreate"));
    nm.connectNodes(grid, 0, attributeCreate, 0);

    nm.getNode(attributeCreate).getParameter("name").lock()->setString("test");
    return attributeCreate;
}

} // namespace

TEST_CASE_METHOD(NMReset, "Every point gets the value when no selection is given")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeCreate = addAttributeCreateAfterGrid();

    nm.getNode(attributeCreate).getParameter("floatValue").lock()->setFloat(2.5f);
    nm.cook(attributeCreate);

    auto& node = nm.getNode(attributeCreate);
    const auto attribute = getAttribute<floatT>(node, attr::AttributeOwner::POINT);
    REQUIRE(attribute.getSize() == getMesh(node)->getNumPoints());
    for (const floatT value : attribute.getAllValues())
        REQUIRE(value == Catch::Approx(2.5f));
}

TEST_CASE_METHOD(NMReset, "A selection limits which points get the value")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeCreate = addAttributeCreateAfterGrid();

    nm.getNode(attributeCreate).getParameter("selection").lock()->setString("p{0}");
    nm.getNode(attributeCreate).getParameter("floatValue").lock()->setFloat(2.5f);
    nm.cook(attributeCreate);

    const auto attribute =
        getAttribute<floatT>(nm.getNode(attributeCreate), attr::AttributeOwner::POINT);
    REQUIRE(attribute.getValue(0) == Catch::Approx(2.5f));
    for (Offset pointOffset = 1; pointOffset < attribute.getSize(); ++pointOffset)
        REQUIRE(attribute.getValue(pointOffset) == Catch::Approx(0.f));
}

TEST_CASE_METHOD(NMReset, "Vectors can be written on faces")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeCreate = addAttributeCreateAfterGrid();

    nm.getNode(attributeCreate).getParameter("attachTo").lock()->setString("face");
    nm.getNode(attributeCreate).getParameter("type").lock()->setString("vector");

    auto vectorValue = nm.getNode(attributeCreate).getParameter("vectorValue").lock();
    vectorValue->setFloat(1.f, 0);
    vectorValue->setFloat(2.f, 1);
    vectorValue->setFloat(3.f, 2);
    nm.cook(attributeCreate);

    auto& node = nm.getNode(attributeCreate);
    const auto attribute = getAttribute<Vector3>(node, attr::AttributeOwner::FACE);
    REQUIRE(attribute.getSize() == getMesh(node)->getNumFaces());
    REQUIRE(attribute.getValue(0) == Vector3(1.f, 2.f, 3.f));
}

TEST_CASE_METHOD(NMReset, "A primitive attribute holds one value")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeCreate = addAttributeCreateAfterGrid();

    nm.getNode(attributeCreate).getParameter("attachTo").lock()->setString("primitive");
    nm.getNode(attributeCreate).getParameter("type").lock()->setString("int");
    nm.getNode(attributeCreate).getParameter("intValue").lock()->setInt(7);
    nm.cook(attributeCreate);

    const auto attribute =
        getAttribute<intT>(nm.getNode(attributeCreate), attr::AttributeOwner::PRIMITIVE);
    REQUIRE(attribute.getSize() == 1);
    REQUIRE(attribute.getValue(0) == 7);
}

TEST_CASE_METHOD(NMReset, "Booleans can be written on vertices")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeCreate = addAttributeCreateAfterGrid();

    nm.getNode(attributeCreate).getParameter("attachTo").lock()->setString("vertex");
    nm.getNode(attributeCreate).getParameter("type").lock()->setString("bool");
    nm.getNode(attributeCreate).getParameter("boolValue").lock()->setInt(1);
    nm.cook(attributeCreate);

    auto& node = nm.getNode(attributeCreate);
    const auto attribute = getAttribute<boolT>(node, attr::AttributeOwner::VERTEX);
    REQUIRE(attribute.getSize() == getMesh(node)->getNumVerts());
    for (const boolT value : attribute.getAllValues())
        REQUIRE(value);
}
