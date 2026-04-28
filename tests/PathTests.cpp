
#include <catch2/catch_test_macros.hpp>
#include "Engine/Operator/Path.h"

TEST_CASE("pathValidation")
{
    using namespace enzo;

    // alphanumeric validation
    Path validAlphaNumeric = "/a/valid/path";
    Path invalidAlphaNumeric = "/a!invalid@/_(path)";
    REQUIRE(validAlphaNumeric.GetName() == "path");
    REQUIRE(validAlphaNumeric.IsValid());
    REQUIRE(!invalidAlphaNumeric.IsValid());

    // double slash validation
    Path doubleSlash = "/a//wrong/path";
    REQUIRE(!doubleSlash.IsValid());

    // valid names
    std::string validName = "my_path";
    std::string invalidName = "!myPath#34";
    REQUIRE(Path::IsValidName(validName));
    REQUIRE(!Path::IsValidName(invalidName));

    // root
    Path root = "/"; // NOTE: Need to handle char conversion
    REQUIRE(root.IsValid());

    // trailing slash
    Path trailingSlash = "/path/to/node/";
    REQUIRE(trailingSlash.IsValid());
    REQUIRE(trailingSlash.IsAbsolute());

    // no root
    Path noRoot = "path/to/node"; // NOTE: This test passes but it should fail?
    REQUIRE(noRoot.IsValid());
    REQUIRE(noRoot.IsRelative());

    // empty string
    Path empty = Path();
    REQUIRE(!empty.IsValid());
}

TEST_CASE("pathGetters")
{
    using namespace enzo;

    // path components
    Path basicPath = "/abs/path/to/object";
    std::vector<std::string> basicPathComponents = basicPath.SplitPath();

    Path relPath = "rel/path";
    std::vector<std::string> relPathComponents = relPath.SplitPath();

    REQUIRE(basicPathComponents.size() == 4);
    REQUIRE(relPathComponents.size() == 2);
    REQUIRE(basicPathComponents[0] == "abs");

    // path prefixes
    std::vector<Path> basicPathPrefixes = basicPath.GetPrefixes();
    std::vector<Path> relPathPrefixes = relPath.GetPrefixes();

    REQUIRE(basicPathPrefixes.size() == 3);
    REQUIRE(relPathPrefixes.size() == 1);
    REQUIRE(basicPathPrefixes[0].GetString() == "/abs");
    REQUIRE(relPathPrefixes[0].GetString() == "rel");

    //path parents
    REQUIRE(basicPath.GetParentPath().GetString() == "/abs/path/to");
    REQUIRE(relPath.GetParentPath().GetString() == "rel");
}

TEST_CASE("pathManipulation")
{
    using namespace enzo;

    // appending to path
    Path basicPath = "/abs/path/to/object";
    std::string path = "/subcomponent/leaf";
    Path appendedPath = basicPath.AppendPath(path);

    REQUIRE(appendedPath.GetString() == "/abs/path/to/object/subcomponent/leaf");
    REQUIRE(basicPath.AppendChild("subcomponent").GetString() == "/abs/path/to/object/subcomponent");

    // incrementing names
    Path mesh = "/geo/mesh";
    Path leaf = "mesh/leaf11";
    
    REQUIRE(mesh.IncrementName() == "/geo/mesh1");
    REQUIRE(leaf.IncrementName() == "mesh/leaf12");

    // abs and rel paths
    Path relativePath = "rel/path";
    Path absPath = "/abs/path/leaf";
    Path anchor = "/abs/path";

    REQUIRE(relativePath.MakeAbsolute() == "/rel/path");
    REQUIRE(absPath.MakeRelative() == "abs/path/leaf");
    REQUIRE(absPath.MakeRelativeTo(anchor) == "leaf");
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
