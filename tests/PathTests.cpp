
#include <catch2/catch_test_macros.hpp>
#include "Engine/Operator/Path.h"

TEST_CASE("pathValidation")
{
    using namespace enzo;

    // alphanumeric validation
    Path validAlphaNumeric = "/a/valid/path";
    Path invalidAlphaNumeric = "/a!invalid@/_(path)";
    REQUIRE(validAlphaNumeric.getName() == "path");
    REQUIRE(validAlphaNumeric.isValid());
    REQUIRE(!invalidAlphaNumeric.isValid());

    // double slash validation
    Path doubleSlash = "/a//wrong/path";
    REQUIRE(!doubleSlash.isValid());

    // valid names
    std::string validName = "my_path";
    std::string invalidName = "!myPath#34";
    REQUIRE(Path::isValidName(validName));
    REQUIRE(!Path::isValidName(invalidName));

    // root
    Path root = "/";
    REQUIRE(root.isValid());

    // trailing slash
    Path trailingSlash = "/path/to/node/";
    REQUIRE(trailingSlash.isValid());
    REQUIRE(trailingSlash.isAbsolute());

    // no root
    Path noRoot = "path/to/node";
    REQUIRE(noRoot.isValid());
    REQUIRE(noRoot.isRelative());

    // empty string
    Path empty = Path();
    REQUIRE(!empty.isValid());
}

TEST_CASE("pathgetters")
{
    using namespace enzo;

    // path components
    Path basicPath = "/abs/path/to/object";
    std::vector<std::string> basicPathComponents = basicPath.split();

    Path relPath = "rel/path";
    std::vector<std::string> relPathComponents = relPath.split();

    REQUIRE(basicPathComponents.size() == 4);
    REQUIRE(relPathComponents.size() == 2);
    REQUIRE(basicPathComponents[0] == "abs");

    // path prefixes
    std::vector<Path> basicPathPrefixes = basicPath.getPrefixes();
    std::vector<Path> relPathPrefixes = relPath.getPrefixes();

    REQUIRE(basicPathPrefixes.size() == 3);
    REQUIRE(relPathPrefixes.size() == 1);
    REQUIRE(basicPathPrefixes[0].getString() == "/abs");
    REQUIRE(relPathPrefixes[0].getString() == "rel");

    //path parents
    REQUIRE(basicPath.getParent().getString() == "/abs/path/to");
    REQUIRE(relPath.getParent().getString() == "rel");
}

TEST_CASE("pathManipulation")
{
    using namespace enzo;

    // appending to path
    Path basicPath = "/abs/path/to/object";
    std::string path = "/subcomponent/leaf";
    Path appendedPath = basicPath.append(path);

    REQUIRE(appendedPath.getString() == "/abs/path/to/object/subcomponent/leaf");
    REQUIRE(basicPath.append("subcomponent").getString() == "/abs/path/to/object/subcomponent");

    // incrementing names
    Path mesh = "/geo/mesh";
    Path leaf = "mesh/leaf11";
    
    REQUIRE(mesh.increment() == "/geo/mesh1");
    REQUIRE(leaf.increment() == "mesh/leaf12");

    // abs and rel paths
    Path relativePath = "rel/path";
    Path absPath = "/abs/path/leaf";
    Path anchor = "/abs/path";

    REQUIRE(relativePath.makeAbsolute() == "/rel/path");
    REQUIRE(absPath.makeRelative() == "abs/path/leaf");
    REQUIRE(absPath.makeRelativeTo(anchor) == "leaf");
}

TEST_CASE("operators")
{
    using namespace enzo;

    Path pathA = "/path/to/something";
    Path pathB = "/path/to/something";
    Path pathC = "path/to/something";

    REQUIRE(pathA == pathB);
    REQUIRE(pathA != pathC);
}
