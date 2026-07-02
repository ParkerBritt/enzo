
#include "Engine/Network/NetworkPath.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("A network path accepts both node and parameter references")
{
    using namespace enzo;

    // A plain path references a node
    NetworkPath node = "/geo/mesh";
    REQUIRE(node.isValid());
    REQUIRE(!node.hasParameter());
    REQUIRE(node.getParameter() == "");

    // A trailing ".name" references a parameter on the final node
    NetworkPath parameter = "/geo/mesh.tx";
    REQUIRE(parameter.isValid());
    REQUIRE(parameter.hasParameter());
    REQUIRE(parameter.getParameter() == "tx");
}

TEST_CASE("A network path inherits the base path component rules")
{
    using namespace enzo;

    REQUIRE(NetworkPath("geo/mesh").isValid());
    REQUIRE(NetworkPath("/").isValid());

    // The node portion still rejects illegal characters and empty components
    REQUIRE(!NetworkPath("/geo//mesh").isValid());
    REQUIRE(!NetworkPath("/geo/me!sh").isValid());
}

TEST_CASE("A network path rejects malformed parameters")
{
    using namespace enzo;

    // A dot with no name after it is not a parameter
    REQUIRE(!NetworkPath("/geo/mesh.").isValid());

    // Only the final component may carry a parameter
    REQUIRE(!NetworkPath("/geo.x/mesh").isValid());

    // A parameter name follows the same character rules as a node name
    REQUIRE(!NetworkPath("/geo/mesh.t!x").isValid());

    // A single dot per path, so a second dot lands inside the parameter name and fails
    REQUIRE(!NetworkPath("/geo/mesh.a.b").isValid());
}

TEST_CASE("A network path strips its parameter to recover the node")
{
    using namespace enzo;

    NetworkPath parameter = "/geo/mesh.tx";
    NetworkPath node = parameter.getNode();

    REQUIRE(node == "/geo/mesh");
    REQUIRE(!node.hasParameter());

    // A node path returns an equivalent copy of itself
    REQUIRE(node.getNode() == "/geo/mesh");
}
