#include "Engine/Dependency/Unit.h"
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>

using namespace enzo;

TEST_CASE("units with the same fields compare equal")
{
    dep::Unit first{7, "translate", 1};
    dep::Unit second{7, "translate", 1};
    REQUIRE(first == second);
}

TEST_CASE("units differing in any field compare unequal")
{
    dep::Unit base{7, "translate", 1};
    REQUIRE_FALSE(base == dep::Unit{8, "translate", 1});
    REQUIRE_FALSE(base == dep::Unit{7, "scale", 1});
    REQUIRE_FALSE(base == dep::Unit{7, "translate", 0});
}

TEST_CASE("a unit knows whether it is a node or a parameter")
{
    dep::Unit node{7};
    dep::Unit parameter{7, "scale", 0};

    REQUIRE(node.isNode());
    REQUIRE_FALSE(node.isParameter());
    REQUIRE(parameter.isParameter());
    REQUIRE_FALSE(parameter.isNode());
}

TEST_CASE("a unit keys an unordered map")
{
    std::unordered_map<dep::Unit, int> values;
    values[{7, "translate", 1}] = 42;

    // The same unit reads back the stored value.
    REQUIRE(values.at({7, "translate", 1}) == 42);

    // A different component is a distinct key.
    REQUIRE(values.find({7, "translate", 0}) == values.end());
}
