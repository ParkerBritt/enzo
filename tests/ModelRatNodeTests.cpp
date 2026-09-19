#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// Creates a rat node at a detail level and cooks it.
std::shared_ptr<const geo::Mesh> cookRat(const std::string& detail)
{
    nt::NodeLoader::loadNodes();
    auto& nm = nt::nm();

    const nt::NodeId nodeId = nm.createNode("enzo::modelRat");
    nt::Node& node = nm.getNode(nodeId);
    node.getParameter("detail").lock()->setString(detail);

    nm.cook(nodeId);

    std::shared_ptr<const enzo::NodePacket> packet = node.getOutputPacket(0);
    REQUIRE(packet->size() == 1);
    return std::dynamic_pointer_cast<const geo::Mesh>(packet->getPrimitive(0));
}

} // namespace

TEST_CASE_METHOD(NMReset, "The simple rat loads its model from the node folder")
{
    const std::shared_ptr<const geo::Mesh> mesh = cookRat("simple");

    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->getNumPoints() == 1940);
    REQUIRE(mesh->getNumFaces() == 1938);
}

TEST_CASE_METHOD(NMReset, "The complex rat loads the higher detail model")
{
    const std::shared_ptr<const geo::Mesh> mesh = cookRat("complex");

    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->getNumPoints() == 12001);
    REQUIRE(mesh->getNumFaces() == 12000);
}
