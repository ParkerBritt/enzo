#include "Engine/Parameter/Parameter.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

static prm::Template dropdownTemplate()
{
    using namespace enzo::prm;
    return Template(Type::DROPDOWN, Name("operation", "Operation"), Default("union"))
        .setOptions({
            Name("union", "Union"),
            Name("intersect", "Intersect"),
            Name("subtract", "Subtract"),
        });
}

TEST_CASE("evalInt returns the index of the selected dropdown option")
{
    prm::Parameter parameter(dropdownTemplate());

    // The default token is the first option.
    REQUIRE(parameter.evalInt() == 0);

    // Selecting a later option returns its position.
    parameter.setString("subtract");
    REQUIRE(parameter.evalInt() == 2);
}

TEST_CASE("evalInt returns minus one when the dropdown token is not an option")
{
    prm::Parameter parameter(dropdownTemplate());

    parameter.setString("nonexistent");
    REQUIRE(parameter.evalInt() == -1);
}

static prm::Template floatTemplate()
{
    using namespace enzo::prm;
    return Template(Type::FLOAT, Name("distance", "Distance"), Default(1));
}

TEST_CASE("evalFloat runs an expression instead of the literal")
{
    prm::Parameter parameter(floatTemplate());

    parameter.setExpression("2 + 3");
    REQUIRE(parameter.hasExpression());
    REQUIRE(parameter.evalFloat() == 5.0f);
}

TEST_CASE("Setting a literal clears the expression")
{
    prm::Parameter parameter(floatTemplate());

    parameter.setExpression("2 + 3");
    parameter.setFloat(9.0f);

    REQUIRE_FALSE(parameter.hasExpression());
    REQUIRE(parameter.evalFloat() == 9.0f);
}

TEST_CASE("A failed expression falls back to the stored literal")
{
    prm::Parameter parameter(floatTemplate());

    parameter.setFloat(4.0f);
    parameter.setExpression("2 +");

    REQUIRE(parameter.evalFloat() == 4.0f);
}

// A ramp template carrying an initial control point count. The ramp supplies
// its own instance fields.
static prm::Template rampTemplate(int points)
{
    using namespace enzo::prm;
    return Template(Type::RAMP, Name("amplitude", "Amplitude"), Default(points));
}

TEST_CASE("A ramp seeds one instance per the parent default count")
{
    prm::Parameter parameter(rampTemplate(2));

    REQUIRE(parameter.getInstanceCount() == 2);

    // The ramp supplies its own fields so the caller never builds them.
    REQUIRE(parameter.getInstanceField(0, "position") != nullptr);
    REQUIRE(parameter.getInstanceField(0, "value") != nullptr);
    REQUIRE(parameter.getInstanceField(0, "interp") != nullptr);
}

TEST_CASE("Adding and removing instances changes the instance count")
{
    prm::Parameter parameter(rampTemplate(2));

    parameter.addInstance();
    REQUIRE(parameter.getInstanceCount() == 3);

    parameter.removeInstance(1);
    REQUIRE(parameter.getInstanceCount() == 2);
}

TEST_CASE("Moving an instance places it at the target index")
{
    prm::Parameter parameter(rampTemplate(3));

    // Tag each instance by its value so the order is observable.
    parameter.getInstanceField(0, "value")->setFloat(0);
    parameter.getInstanceField(1, "value")->setFloat(1);
    parameter.getInstanceField(2, "value")->setFloat(2);

    parameter.moveInstance(0, 2);

    REQUIRE(parameter.getInstanceField(0, "value")->evalFloat() == 1);
    REQUIRE(parameter.getInstanceField(1, "value")->evalFloat() == 2);
    REQUIRE(parameter.getInstanceField(2, "value")->evalFloat() == 0);
}

TEST_CASE("Editing an instance field notifies the parent parameter")
{
    prm::Parameter parameter(rampTemplate(2));

    bool notified = false;
    parameter.valueChanged.connect([&notified]() { notified = true; });

    parameter.getInstanceField(0, "value")->setFloat(5);

    REQUIRE(notified);
}
