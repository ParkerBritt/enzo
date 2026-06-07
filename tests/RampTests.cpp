#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/Ramp.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {

prm::Ramp::Key makeKey(
    floatT position, floatT value, prm::Interpolation interp = prm::Interpolation::LINEAR
)
{
    return {position, value, interp};
}

} // namespace

TEST_CASE("Sampling an empty ramp returns zero")
{
    prm::Ramp ramp;
    REQUIRE(ramp.sample(0.5) == 0);
}

TEST_CASE("Sampling outside the ramp domain clamps to the end keys")
{
    prm::Ramp ramp(std::vector<prm::Ramp::Key>{makeKey(0.2, 1), makeKey(0.8, 5)});

    REQUIRE(ramp.sample(0.0) == 1);
    REQUIRE(ramp.sample(1.0) == 5);
}

TEST_CASE("Linear interpolation blends between neighbouring keys")
{
    prm::Ramp ramp(std::vector<prm::Ramp::Key>{makeKey(0, 0), makeKey(1, 10)});

    REQUIRE(ramp.sample(0.5) == 5);
    REQUIRE(ramp.sample(0.25) == 2.5);
}

TEST_CASE("Constant interpolation holds the left key value across the segment")
{
    prm::Ramp ramp(std::vector<prm::Ramp::Key>{
        makeKey(0, 3, prm::Interpolation::CONSTANT),
        makeKey(1, 9),
    });

    REQUIRE(ramp.sample(0.0) == 3);
    REQUIRE(ramp.sample(0.99) == 3);
}

TEST_CASE("A ramp snapshot sorts the control points by position")
{
    using namespace enzo::prm;
    Parameter parameter(Template(Type::RAMP, Name("amplitude", "Amplitude"), Default(2)));

    // Seed two points out of order so the snapshot must sort them by position.
    parameter.getInstanceField(0, "position")->setFloat(1);
    parameter.getInstanceField(0, "value")->setFloat(10);
    parameter.getInstanceField(1, "position")->setFloat(0);
    parameter.getInstanceField(1, "value")->setFloat(0);

    const Ramp ramp(parameter);

    REQUIRE(ramp.size() == 2);
    REQUIRE(ramp.sample(0.5) == 5);
}
