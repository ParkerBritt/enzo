#include "Engine/Core/Types.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// An obj file that lives only for the length of one test case.
class TemporaryObjFile
{
  public:
    TemporaryObjFile(const std::string& fileName, const std::string& contents)
        : path_(std::filesystem::temp_directory_path() / fileName)
    {
        std::ofstream file(path_);
        file << contents;
    }
    ~TemporaryObjFile() { std::filesystem::remove(path_); }

    const std::filesystem::path& getPath() const { return path_; }

  private:
    std::filesystem::path path_;
};

// Creates a geometry import node pointed at a file and cooks it.
std::shared_ptr<const geo::Mesh> importMesh(const std::filesystem::path& file, floatT size = 1.f)
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    const nt::NodeId nodeId = nm.createNode(nt::NodeTypeTable::requireNodeType("geometryImport"));
    nt::Node& node = nm.getNode(nodeId);
    node.getParameter("filePath").lock()->setString(file.string());
    node.getParameter("size").lock()->setFloat(size);

    nm.cook(nodeId);

    std::shared_ptr<const enzo::NodePacket> packet = node.getOutputPacket(0);
    REQUIRE(packet->size() == 1);
    return std::dynamic_pointer_cast<const geo::Mesh>(packet->getPrimitive(0));
}

// A square built from four vertices, with the face declared in the three field
// form an exporter writes when it also stores texture and normal indices.
constexpr const char* kSquareObj = R"(# a square
v -1.0 0.0 -1.0
v  1.0 0.0 -1.0
v  1.0 0.0  1.0
v -1.0 0.0  1.0
vn 0.0 1.0 0.0
f 1/1/1 2/2/1 3/3/1 4/4/1
)";

} // namespace

TEST_CASE_METHOD(NMReset, "Importing an obj file builds its points and faces")
{
    const TemporaryObjFile file("enzoGeometryImportSquare.obj", kSquareObj);

    const std::shared_ptr<const geo::Mesh> mesh = importMesh(file.getPath());

    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->getNumPoints() == 4);
    REQUIRE(mesh->getNumFaces() == 1);
    REQUIRE(mesh->getPointPos(0) == Vector3(-1.f, 0.f, -1.f));
    REQUIRE(mesh->getPointPos(2) == Vector3(1.f, 0.f, 1.f));
}

TEST_CASE_METHOD(NMReset, "The size parameter scales the imported points")
{
    const TemporaryObjFile file("enzoGeometryImportScaled.obj", kSquareObj);

    const std::shared_ptr<const geo::Mesh> mesh = importMesh(file.getPath(), 2.f);

    REQUIRE(mesh->getPointPos(2) == Vector3(2.f, 0.f, 2.f));
}

TEST_CASE_METHOD(NMReset, "An open line record makes an unclosed face")
{
    const TemporaryObjFile file(
        "enzoGeometryImportLine.obj",
        "v 0 0 0\nv 1 0 0\nv 2 0 0\nl 1 2 3\n"
    );

    const std::shared_ptr<const geo::Mesh> mesh = importMesh(file.getPath());

    REQUIRE(mesh->getNumPoints() == 3);
    REQUIRE(mesh->getNumFaces() == 1);
    // A closed triangle would carry a vertex back to the first point
    REQUIRE(mesh->getNumVerts() == 3);
}

TEST_CASE_METHOD(NMReset, "Negative obj indices count back from the last point")
{
    const TemporaryObjFile file(
        "enzoGeometryImportNegative.obj",
        "v 0 0 0\nv 1 0 0\nv 2 0 0\nf -3 -2 -1\n"
    );

    const std::shared_ptr<const geo::Mesh> mesh = importMesh(file.getPath());

    REQUIRE(mesh->getNumFaces() == 1);
    REQUIRE(mesh->getNumVerts() == 3);
}

TEST_CASE_METHOD(NMReset, "An unset file path imports nothing")
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    const nt::NodeId nodeId = nm.createNode(nt::NodeTypeTable::requireNodeType("geometryImport"));
    nm.cook(nodeId);

    REQUIRE(nm.getNode(nodeId).getOutputPacket(0)->size() == 1);
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(nm.getNode(nodeId).getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh->getNumPoints() == 0);
}
