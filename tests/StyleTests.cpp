#include "Engine/Parameter/StyleAccess.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("Owners read as a space separated list of names")
{
    const std::vector<attr::AttributeOwner> owners = prm::style::parseOwners("point vertex");

    REQUIRE(owners == std::vector{attr::AttributeOwner::POINT, attr::AttributeOwner::VERTEX});
}

TEST_CASE("The owner name all stands for every part of the geometry")
{
    const std::vector<attr::AttributeOwner> owners = prm::style::parseOwners("all");

    REQUIRE(owners.size() == 4);
}

TEST_CASE("An unknown owner name is rejected")
{
    REQUIRE_THROWS_AS(prm::style::parseOwners("corner"), std::runtime_error);
}

TEST_CASE("Attribute types read as a space separated list of names")
{
    const std::vector<attr::AttributeType> types = prm::style::parseAttributeTypes("float vector");

    REQUIRE(types == std::vector{attr::AttributeType::floatT, attr::AttributeType::vectorT});
}

TEST_CASE("The attribute type name all stands for every type")
{
    const std::vector<attr::AttributeType> types = prm::style::parseAttributeTypes("all");

    REQUIRE(types.size() == 6);
}

TEST_CASE("An unknown attribute type name is rejected")
{
    REQUIRE_THROWS_AS(prm::style::parseAttributeTypes("quaternion"), std::runtime_error);
}

TEST_CASE("A option written as a call names the parameter it reads")
{
    REQUIRE(prm::style::getReferencedParameterName("prm(attachTo)") == "attachTo");
}

TEST_CASE("A option written as a literal reads no parameter")
{
    REQUIRE_FALSE(prm::style::getReferencedParameterName("point vertex").has_value());
    REQUIRE_FALSE(prm::style::getReferencedParameterName("all").has_value());
}
