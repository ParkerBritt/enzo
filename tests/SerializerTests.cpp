#include "Engine/Parameter/Parameter.h"
#include "Engine/Serializer/ParameterSerializable.h"
#include <catch2/catch_test_macros.hpp>

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
