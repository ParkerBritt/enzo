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
