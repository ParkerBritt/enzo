#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeType.h"
#include "Engine/Network/NodeTypeTable.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <vector>

using namespace enzo;

namespace {

// Returns every folder that claims to be a node, read straight off disk rather
// than from the table, so a node the loader dropped still shows up here.
std::vector<std::string> shippedNodeNames()
{
    std::vector<std::string> names;
    for (const auto& entry :
         std::filesystem::directory_iterator(nt::NodeLoader::getNodesDirectory()))
        if (std::filesystem::exists(entry.path() / "node.yaml"))
            names.push_back(entry.path().filename().string());
    return names;
}

} // namespace

TEST_CASE("Every shipped node loads and resolves its implementation")
{
    nt::NodeLoader::loadNodes();

    // A node is declared in yaml rather than C++, so a broken one only shows up
    // at runtime. This is the check that catches it.
    for (const std::string& name : shippedNodeNames())
    {
        INFO("node " << name);

        // Every shipped node is published under the enzo namespace.
        const nt::NodeType* nodeType = nt::NodeTypeTable::getNodeType("enzo::" + name);
        REQUIRE(nodeType != nullptr);
        REQUIRE(nodeType->ctorFunc != nullptr);
        REQUIRE(nodeType->folder.filename() == name);
    }
}

TEST_CASE("Loading twice leaves one entry per node")
{
    nt::NodeLoader::loadNodes();
    const size_t loadedCount = nt::NodeTypeTable::getData().size();

    nt::NodeLoader::loadNodes();

    REQUIRE(nt::NodeTypeTable::getData().size() == loadedCount);

    // Other tests register their own node types in the shared table, so a shipped node is
    // checked for a single entry by name
    for (const std::string& name : shippedNodeNames())
    {
        INFO("node " << name);

        const std::string fullName = "enzo::" + name;
        size_t entryCount = 0;
        for (const nt::NodeType& nodeType : nt::NodeTypeTable::getData())
            if (nodeType.getFullName() == fullName) entryCount++;

        REQUIRE(entryCount == 1);
    }
}
