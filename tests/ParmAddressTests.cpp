#include "Engine/Parameter/ParmAddress.h"
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>

using namespace enzo;

TEST_CASE("addresses with the same fields compare equal")
{
    prm::ParmAddress first{7, "translate", 1};
    prm::ParmAddress second{7, "translate", 1};
    REQUIRE(first == second);
}

TEST_CASE("addresses differing in any field compare unequal")
{
    prm::ParmAddress base{7, "translate", 1};
    REQUIRE_FALSE(base == prm::ParmAddress{8, "translate", 1});
    REQUIRE_FALSE(base == prm::ParmAddress{7, "scale", 1});
    REQUIRE_FALSE(base == prm::ParmAddress{7, "translate", 0});
}

TEST_CASE("an address keys an unordered map")
{
    std::unordered_map<prm::ParmAddress, int> values;
    values[{7, "translate", 1}] = 42;

    // The same address reads back the stored value.
    REQUIRE(values.at({7, "translate", 1}) == 42);

    // A different component is a distinct key.
    REQUIRE(values.find({7, "translate", 0}) == values.end());
}
