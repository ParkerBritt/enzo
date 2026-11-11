#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Gui/Components/IconFiles.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("An icon name finds a Lucide icon")
{
    REQUIRE(ui::getIconFile("plus").filename() == "plus.svg");
}

TEST_CASE("A node icon name finds the node's own icon")
{
    nt::NodeLoader::loadNodes();
    const nt::NodeType& gridType = nt::NodeTypeTable::requireNodeType("enzo::grid");

    REQUIRE(ui::getIconFile("node/enzo::grid") == gridType.getIconFile());
}

TEST_CASE("An icon name nothing carries finds nothing")
{
    REQUIRE(ui::getIconFile("noSuchIcon").empty());
    REQUIRE(ui::getIconFile("node/enzo::noSuchNode").empty());
}

TEST_CASE("A node with no icon of its own finds the default node icon")
{
    nt::NodeLoader::loadNodes();
    nt::NodeType iconlessType = nt::NodeTypeTable::requireNodeType("enzo::grid");
    iconlessType.iconPath.clear();

    REQUIRE(ui::getNodeIconFile(iconlessType).filename() == "nodeDefault.svg");
}
