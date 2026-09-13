#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <set>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// The geometry, seed and fracture nodes of a fracture network.
struct GridAndSeeds
{
    nt::NodeId geometry;
    nt::NodeId seeds;
    nt::NodeId fracture;
};

// Returns the mesh a cooked node put on its first output.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Sets a grid's point counts and makes it two units wide and deep.
void setGridShape(nt::Node& grid, intT columns, intT rows)
{
    grid.getParameter("columns").lock()->setInt(columns);
    grid.getParameter("rows").lock()->setInt(rows);
    grid.getParameter("size").lock()->setFloat(2.f, 0);
    grid.getParameter("size").lock()->setFloat(2.f, 1);
}

// Builds a single quad spanning x and z from -1 to 1, with seed points at x equal to -1 and 1.
GridAndSeeds addFractureOfQuad()
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    GridAndSeeds nodes;
    nodes.geometry = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nodes.seeds = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nodes.fracture = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cellFracture"));
    nm.connectNodes(nodes.geometry, 0, nodes.fracture, 0);
    nm.connectNodes(nodes.seeds, 0, nodes.fracture, 1);

    setGridShape(nm.getNode(nodes.geometry), 2, 2);
    setGridShape(nm.getNode(nodes.seeds), 2, 1);

    return nodes;
}

// Builds a unit cube fractured around the points of a grid.
GridAndSeeds addFractureOfCube(intT seedColumns, intT seedRows)
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    GridAndSeeds nodes;
    nodes.geometry = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cube"));
    nodes.seeds = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nodes.fracture = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cellFracture"));
    nm.connectNodes(nodes.geometry, 0, nodes.fracture, 0);
    nm.connectNodes(nodes.seeds, 0, nodes.fracture, 1);

    setGridShape(nm.getNode(nodes.seeds), seedColumns, seedRows);

    return nodes;
}

// Returns whether every edge is shared by exactly two faces running along it in opposite
// directions.
bool isClosedSurface(const geo::Mesh& mesh)
{
    std::set<std::pair<Offset, Offset>> directedEdges;
    for (const Offset faceOffset : mesh.getFaces())
    {
        const auto facePoints = mesh.getFacePoints(faceOffset);
        for (size_t cornerIndex = 0; cornerIndex < facePoints.size(); ++cornerIndex)
        {
            const Offset startPoint = facePoints[cornerIndex];
            const Offset endPoint = facePoints[(cornerIndex + 1) % facePoints.size()];
            if (!directedEdges.insert({startPoint, endPoint}).second) return false;
        }
    }
    for (const auto& [startPoint, endPoint] : directedEdges)
        if (!directedEdges.contains({endPoint, startPoint})) return false;
    return true;
}

// Returns how many faces belong to the named face group, or -1 when there is no such group.
int countGroupFaces(const geo::Mesh& mesh, const std::string& groupName)
{
    auto group = mesh.getGroupByName(attr::AttributeOwner::FACE, groupName);
    if (!group) return -1;
    attr::AttributeHandleRO<boolT> membership(group);
    int memberCount = 0;
    for (Offset faceOffset = 0; faceOffset < mesh.getNumFaces(); ++faceOffset)
        if (membership.getValue(faceOffset)) ++memberCount;
    return memberCount;
}

} // namespace

TEST_CASE_METHOD(NMReset, "Two seeds split a cube into two closed pieces")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfCube(2, 1);
    nm.cook(nodes.fracture);

    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(isClosedSurface(*fractured));
    REQUIRE(countGroupFaces(*fractured, "inside") == 2);
}

TEST_CASE_METHOD(NMReset, "Four seeds split a cube into four closed pieces")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfCube(2, 2);
    nm.cook(nodes.fracture);

    // Each quarter is closed by two caps, one of which a later plane cuts in half.
    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(isClosedSurface(*fractured));
    REQUIRE(countGroupFaces(*fractured, "inside") == 8);
}

TEST_CASE_METHOD(NMReset, "Fracturing an open surface leaves the inside group empty")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfQuad();
    nm.cook(nodes.fracture);

    REQUIRE(countGroupFaces(*getMesh(nm.getNode(nodes.fracture)), "inside") == 0);
}

TEST_CASE_METHOD(NMReset, "An empty inside group name still caps without adding a group")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfCube(2, 1);
    nm.getNode(nodes.fracture).getParameter("insideGroup").lock()->setString("");
    nm.cook(nodes.fracture);

    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(isClosedSurface(*fractured));
    REQUIRE(fractured->getNumGroups(attr::AttributeOwner::FACE) == 0);
}

TEST_CASE_METHOD(NMReset, "Two seeds split a quad into a piece on each side")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfQuad();
    nm.cook(nodes.fracture);

    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(fractured->getNumFaces() == 2);

    auto pieceAttribute = fractured->getAttribByName(attr::AttributeOwner::FACE, "piece");
    REQUIRE(pieceAttribute != nullptr);
    attr::AttributeHandleRO<intT> piece(pieceAttribute);

    // The first seed sits at x equal to -1, so its piece lies at or below x equal to 0.
    std::set<intT> pieceNumbers;
    for (Offset faceOffset = 0; faceOffset < fractured->getNumFaces(); ++faceOffset)
    {
        const intT pieceNumber = piece.getValue(faceOffset);
        pieceNumbers.insert(pieceNumber);
        for (const intT pointOffset : fractured->getFacePoints(faceOffset))
        {
            const float x = fractured->getPointPos(pointOffset).x();
            if (pieceNumber == 0) REQUIRE(x <= 1e-5f);
            if (pieceNumber == 1) REQUIRE(x >= -1e-5f);
        }
    }
    REQUIRE(pieceNumbers == std::set<intT>{0, 1});
}

TEST_CASE_METHOD(NMReset, "A single seed keeps the whole geometry as piece zero")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfQuad();
    setGridShape(nm.getNode(nodes.seeds), 1, 1);
    nm.cook(nodes.fracture);

    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(fractured->getNumFaces() == 1);
    REQUIRE(fractured->getNumPoints() == 4);

    auto pieceAttribute = fractured->getAttribByName(attr::AttributeOwner::FACE, "piece");
    REQUIRE(pieceAttribute != nullptr);
    REQUIRE(attr::AttributeHandleRO<intT>(pieceAttribute).getValue(0) == 0);
}

TEST_CASE_METHOD(NMReset, "Geometry without seeds passes through unchanged")
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    const nt::NodeId geometry = nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
    const nt::NodeId fracture =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::cellFracture"));
    nm.connectNodes(geometry, 0, fracture, 0);
    setGridShape(nm.getNode(geometry), 2, 2);
    nm.cook(fracture);

    const auto fractured = getMesh(nm.getNode(fracture));
    REQUIRE(fractured->getNumFaces() == 1);
    REQUIRE(fractured->getAttribByName(attr::AttributeOwner::FACE, "piece") == nullptr);
}

TEST_CASE_METHOD(NMReset, "An empty piece attribute name adds no attribute")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfQuad();
    nm.getNode(nodes.fracture).getParameter("pieceAttribute").lock()->setString("");
    nm.cook(nodes.fracture);

    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(fractured->getNumFaces() == 2);
    REQUIRE(fractured->getAttribByName(attr::AttributeOwner::FACE, "") == nullptr);
    REQUIRE(
        fractured->getNumAttributes(attr::AttributeOwner::FACE) ==
        getMesh(nm.getNode(nodes.geometry))->getNumAttributes(attr::AttributeOwner::FACE)
    );
}

TEST_CASE_METHOD(NMReset, "Seeds in the same place do not duplicate the geometry")
{
    auto& nm = nt::nm();
    const GridAndSeeds nodes = addFractureOfCube(2, 1);

    // A grid with no size puts both of its points at the origin.
    nm.getNode(nodes.seeds).getParameter("size").lock()->setFloat(0.f, 0);
    nm.getNode(nodes.seeds).getParameter("size").lock()->setFloat(0.f, 1);
    nm.cook(nodes.fracture);

    const auto fractured = getMesh(nm.getNode(nodes.fracture));
    REQUIRE(fractured->getNumFaces() == getMesh(nm.getNode(nodes.geometry))->getNumFaces());
}
