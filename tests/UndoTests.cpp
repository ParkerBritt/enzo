#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
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
        op::OperatorTable::initPlugins();
        nt::nm()._reset();
    }
    ~PluginsAndReset() { nt::nm()._reset(); }
};

} // namespace

TEST_CASE_METHOD(PluginsAndReset, "Undo and redo restore a ramp field edit")
{
    auto& networkManager = nt::nm();
    nt::OpId opId = networkManager.createOperator(op::OperatorTable::getOpInfo("sineWave").value());

    auto amplitude = networkManager.getGeoOperator(opId).getParameter("amplitude").lock();
    REQUIRE(amplitude);

    // The default amplitude ramp is a linear zero to one curve.
    REQUIRE(amplitude->getInstanceField(0, "value")->evalFloat() == 0);

    ParameterSerializable before = toSerializable(*amplitude);
    amplitude->getInstanceField(0, "value")->setFloat(0.7f);
    ParameterSerializable after = toSerializable(*amplitude);

    nt::ChangeParameterCommand command(opId, "amplitude", before, after);

    command.undo();
    REQUIRE(amplitude->getInstanceField(0, "value")->evalFloat() == 0);

    command.redo();
    REQUIRE(amplitude->getInstanceField(0, "value")->evalFloat() == 0.7f);
}

TEST_CASE_METHOD(PluginsAndReset, "Undo restores a removed ramp control point")
{
    auto& networkManager = nt::nm();
    nt::OpId opId = networkManager.createOperator(op::OperatorTable::getOpInfo("sineWave").value());

    auto amplitude = networkManager.getGeoOperator(opId).getParameter("amplitude").lock();
    REQUIRE(amplitude);

    ParameterSerializable before = toSerializable(*amplitude);
    amplitude->addInstance();
    amplitude->getInstanceField(2, "position")->setFloat(0.5f);
    ParameterSerializable after = toSerializable(*amplitude);

    nt::ChangeParameterCommand command(opId, "amplitude", before, after);

    command.undo();
    REQUIRE(amplitude->getInstanceCount() == 2);

    command.redo();
    REQUIRE(amplitude->getInstanceCount() == 3);
    REQUIRE(amplitude->getInstanceField(2, "position")->evalFloat() == 0.5f);
}
