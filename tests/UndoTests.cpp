#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include "Engine/UndoRedo/ChangeParameterCommand.h"
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

} // namespace

TEST_CASE_METHOD(PluginsAndReset, "Undo and redo restore a ramp field edit")
{
    auto& networkManager = nt::nm();
    nt::NodeId nodeId =
        networkManager.createNode(nt::NodeTypeTable::requireNodeType("enzo::sineWave"));

    auto amplitude = networkManager.getNode(nodeId).getParameter("amplitude").lock();
    REQUIRE(amplitude);

    // The default amplitude ramp is a linear zero to one curve.
    REQUIRE(amplitude->getInstanceField(0, "value")->evalFloat() == 0);

    ParameterSerializable before = toSerializable(*amplitude);
    amplitude->getInstanceField(0, "value")->setFloat(0.7f);
    ParameterSerializable after = toSerializable(*amplitude);

    nt::ChangeParameterCommand command(nodeId, "amplitude", before, after);

    command.undo();
    REQUIRE(amplitude->getInstanceField(0, "value")->evalFloat() == 0);

    command.redo();
    REQUIRE(amplitude->getInstanceField(0, "value")->evalFloat() == 0.7f);
}

TEST_CASE_METHOD(PluginsAndReset, "Undo restores a removed ramp control point")
{
    auto& networkManager = nt::nm();
    nt::NodeId nodeId =
        networkManager.createNode(nt::NodeTypeTable::requireNodeType("enzo::sineWave"));

    auto amplitude = networkManager.getNode(nodeId).getParameter("amplitude").lock();
    REQUIRE(amplitude);

    ParameterSerializable before = toSerializable(*amplitude);
    amplitude->addInstance();
    amplitude->getInstanceField(2, "position")->setFloat(0.5f);
    ParameterSerializable after = toSerializable(*amplitude);

    nt::ChangeParameterCommand command(nodeId, "amplitude", before, after);

    command.undo();
    REQUIRE(amplitude->getInstanceCount() == 2);

    command.redo();
    REQUIRE(amplitude->getInstanceCount() == 3);
    REQUIRE(amplitude->getInstanceField(2, "position")->evalFloat() == 0.5f);
}

TEST_CASE_METHOD(PluginsAndReset, "Undoing a delete restores a parameter expression")
{
    auto& networkManager = nt::nm();
    nt::NodeId nodeId =
        networkManager.createNode(nt::NodeTypeTable::requireNodeType("enzo::sineWave"));

    auto frequency = networkManager.getNode(nodeId).getParameter("frequency").lock();
    REQUIRE(frequency);

    frequency->setExpression("2 + 3");
    REQUIRE(frequency->evalFloat() == 5);

    networkManager.deleteNode(nodeId);
    networkManager.undoStack().undo();

    // The node returns under the id it had, so the same handle finds it again.
    auto restored = networkManager.getNode(nodeId).getParameter("frequency").lock();
    REQUIRE(restored);
    REQUIRE(restored->getExpression() == "2 + 3");
    REQUIRE(restored->evalFloat() == 5);
}

TEST_CASE_METHOD(PluginsAndReset, "Undoing a delete restores added ramp control points")
{
    auto& networkManager = nt::nm();
    nt::NodeId nodeId =
        networkManager.createNode(nt::NodeTypeTable::requireNodeType("enzo::sineWave"));

    auto amplitude = networkManager.getNode(nodeId).getParameter("amplitude").lock();
    REQUIRE(amplitude);

    // A third control point, one past the linear zero to one default.
    amplitude->addInstance();
    amplitude->getInstanceField(2, "position")->setFloat(0.5f);
    REQUIRE(amplitude->getInstanceCount() == 3);

    networkManager.deleteNode(nodeId);
    networkManager.undoStack().undo();

    auto restored = networkManager.getNode(nodeId).getParameter("amplitude").lock();
    REQUIRE(restored);
    REQUIRE(restored->getInstanceCount() == 3);
    REQUIRE(restored->getInstanceField(2, "position")->evalFloat() == 0.5f);
}
