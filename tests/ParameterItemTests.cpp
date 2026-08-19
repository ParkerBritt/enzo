#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Gui/Parameters/ParameterItem.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {

struct PluginsAndReset
{
    PluginsAndReset()
    {
        nt::NodeLoader::loadNodes();
        nt::nm()._reset();
    }
    ~PluginsAndReset() { nt::nm()._reset(); }
};

/// @brief Builds the item QML would hold for one of a node's parameters.
ui::ParameterItem makeItem(nt::Node& node, const std::string& parmName)
{
    auto parameter = node.getParameter(parmName).lock();
    REQUIRE(parameter);
    return ui::ParameterItem(parameter->getTemplate(), parameter, node);
}

} // namespace

TEST_CASE_METHOD(PluginsAndReset, "Undo restores a toggle edited through its item")
{
    auto& networkManager = nt::nm();
    nt::NodeId nodeId =
        networkManager.createNode(nt::NodeTypeTable::requireNodeType("enzo::extrude"));
    nt::Node& node = networkManager.getNode(nodeId);

    ui::ParameterItem frontOutput = makeItem(node, "frontOutput");
    REQUIRE(frontOutput.value().toInt() == 1);

    networkManager.undoStack().clear();

    // The toggle switch flips the value inside one begin and commit pair.
    frontOutput.beginEdit();
    frontOutput.setValue(0);
    frontOutput.commitEdit();
    REQUIRE(frontOutput.value().toInt() == 0);

    networkManager.undoStack().undo();
    REQUIRE(frontOutput.value().toInt() == 1);
}

TEST_CASE_METHOD(PluginsAndReset, "A drag over many values records one undo step")
{
    auto& networkManager = nt::nm();
    nt::NodeId nodeId =
        networkManager.createNode(nt::NodeTypeTable::requireNodeType("enzo::extrude"));
    nt::Node& node = networkManager.getNode(nodeId);

    ui::ParameterItem distance = makeItem(node, "distance");
    const double original = distance.value().toDouble();

    networkManager.undoStack().clear();

    // A slider drag writes a value per motion sample between the begin and commit.
    distance.beginEdit();
    distance.setValue(0.25);
    distance.setValue(0.5);
    distance.setValue(0.75);
    distance.commitEdit();
    REQUIRE(distance.value().toDouble() == 0.75);

    networkManager.undoStack().undo();
    REQUIRE(distance.value().toDouble() == original);
    REQUIRE_FALSE(networkManager.undoStack().canUndo());
}
