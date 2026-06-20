#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include "Engine/Serializer/Serializer.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>

using namespace enzo;

namespace {

prm::Parameter makeRamp(int pointCount)
{
    return prm::Parameter(
        prm::Template(
            prm::Type::RAMP,
            prm::Name("amplitude", "Amplitude"),
            prm::Default(pointCount)
        )
    );
}

struct NMReset
{
    NMReset() { enzo::nt::nm()._reset(); }
    ~NMReset() { enzo::nt::nm()._reset(); }
};

struct OperatorTableInit
{
    OperatorTableInit() { enzo::op::OperatorTable::initPlugins(); }
};
static OperatorTableInit _operatorTableInit;

} // namespace

TEST_CASE("A flat parameter round trips through the serializable model")
{
    prm::Parameter source(
        prm::Template(prm::Type::FLOAT, prm::Name("frequency", "Frequency"), prm::Default(0))
    );
    source.setFloat(3.5);

    ParameterSerializable model = toSerializable(source);

    prm::Parameter restored(
        prm::Template(prm::Type::FLOAT, prm::Name("frequency", "Frequency"), prm::Default(0))
    );
    applySerializable(restored, model);

    REQUIRE(restored.evalFloat() == 3.5f);
}

TEST_CASE("A ramp serializes its instance fields without touching the flat value")
{
    prm::Parameter source = makeRamp(2);
    source.getInstanceField(0, "position")->setFloat(0);
    source.getInstanceField(0, "value")->setFloat(2);
    source.getInstanceField(1, "position")->setFloat(1);
    source.getInstanceField(1, "value")->setFloat(8);
    source.getInstanceField(1, "interp")->setString("constant");

    ParameterSerializable model = toSerializable(source);

    REQUIRE(model.instances.size() == 2);
    REQUIRE(model.floatValues.empty());
    REQUIRE(model.intValues.empty());
}

TEST_CASE("A ramp round trips its per point position value and interpolation")
{
    prm::Parameter source = makeRamp(2);
    source.getInstanceField(0, "position")->setFloat(0);
    source.getInstanceField(0, "value")->setFloat(2);
    source.getInstanceField(1, "position")->setFloat(1);
    source.getInstanceField(1, "value")->setFloat(8);
    source.getInstanceField(1, "interp")->setString("constant");

    ParameterSerializable model = toSerializable(source);

    prm::Parameter restored = makeRamp(2);
    applySerializable(restored, model);

    REQUIRE(restored.getInstanceCount() == 2);
    REQUIRE(restored.getInstanceField(0, "value")->evalFloat() == 2);
    REQUIRE(restored.getInstanceField(1, "position")->evalFloat() == 1);
    REQUIRE(restored.getInstanceField(1, "value")->evalFloat() == 8);
    REQUIRE(restored.getInstanceField(1, "interp")->evalString() == "constant");
}

TEST_CASE("Applying a ramp model reconciles a mismatched instance count")
{
    prm::Parameter source = makeRamp(4);
    ParameterSerializable grown = toSerializable(source);

    // A freshly created ramp starts at its own default count and must resize to
    // match whatever the model carries, both up and down.
    prm::Parameter restoredUp = makeRamp(1);
    applySerializable(restoredUp, grown);
    REQUIRE(restoredUp.getInstanceCount() == 4);

    prm::Parameter single = makeRamp(1);
    ParameterSerializable shrunk = toSerializable(single);
    prm::Parameter restoredDown = makeRamp(3);
    applySerializable(restoredDown, shrunk);
    REQUIRE(restoredDown.getInstanceCount() == 1);
}

TEST_CASE_METHOD(NMReset, "A connection round trips through save and load")
{
    auto& nm = nt::nm();
    auto gridInfo = op::OperatorTable::getOpInfo("grid").value();
    auto transformInfo = op::OperatorTable::getOpInfo("transform").value();

    nt::OpId grid = nm.createOperator(gridInfo);
    nt::OpId transform = nm.createOperator(transformInfo);
    nt::nm().connectNodes(grid, 0, transform, 0);

    const std::string path = "/tmp/enzo_serializer_roundtrip.json";
    nt::Serializer serializer;
    serializer.save(nm, path);

    nm._reset();
    serializer.load(nm, path);

    // Find the reloaded nodes by type since load assigns fresh ids
    std::optional<nt::OpId> loadedGrid;
    std::optional<nt::OpId> loadedTransform;
    for (auto [opId, op] : nm.operators())
    {
        if (op.getType().getName() == "grid") loadedGrid = opId;
        if (op.getType().getName() == "transform") loadedTransform = opId;
    }
    REQUIRE(loadedGrid.has_value());
    REQUIRE(loadedTransform.has_value());

    // The transform reads its single input from the grid
    std::vector<nt::Connection> inputs = nm.graph().getInputs(loadedTransform.value());
    REQUIRE(inputs.size() == 1);
    REQUIRE(inputs[0].sourceOp == loadedGrid.value());
}
