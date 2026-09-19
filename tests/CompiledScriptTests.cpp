#include "Engine/Expression/DasRuntime.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {
std::shared_ptr<expr::CompiledScript> compileScript(const String& source)
{
    String error;
    auto script = expr::DasRuntime::instance().compile("compiledScriptTest", source, error);
    INFO(error);
    REQUIRE(script);
    return script;
}
} // namespace

TEST_CASE("run passes an integer and a reference to the function")
{
    auto script = compileScript(R"(options gen2
[export]
def double(value : int64; var result : int64&) {
    result = value * 2l
}
)");

    intT result = 0;
    const expr::ScriptArgument arguments[] = {intT(21), &result};
    String error;
    REQUIRE(script->run("double", arguments, nullptr, error));
    REQUIRE(result == 42);
}

TEST_CASE("run reports a panic in the function")
{
    auto script = compileScript(R"(options gen2
[export]
def fail(value : int64) {
    panic("failed on {value}")
}
)");

    const expr::ScriptArgument arguments[] = {intT(3)};
    String error;
    REQUIRE_FALSE(script->run("fail", arguments, nullptr, error));
    REQUIRE(error.find("failed on 3") != String::npos);
}

TEST_CASE("run reports a function that does not exist")
{
    auto script = compileScript(R"(options gen2
[export]
def present() {
}
)");

    String error;
    REQUIRE_FALSE(script->run("missing", {}, nullptr, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("run reports more arguments than daslang allows")
{
    auto script = compileScript(R"(options gen2
[export]
def present() {
}
)");

    const std::vector<expr::ScriptArgument> arguments(expr::maxScriptArguments + 1, intT(0));
    String error;
    REQUIRE_FALSE(script->run("present", arguments, nullptr, error));
    REQUIRE(error.find("too many arguments") != String::npos);
}

TEST_CASE("clone runs with its own globals")
{
    auto script = compileScript(R"(options gen2
var count : int64 = 0l

[export]
def countCalls(var result : int64&) {
    count += 1l
    result = count
}
)");

    // Runs the original twice, leaving its count at 2
    intT result = 0;
    const expr::ScriptArgument arguments[] = {&result};
    String error;
    REQUIRE(script->run("countCalls", arguments, nullptr, error));
    REQUIRE(script->run("countCalls", arguments, nullptr, error));
    REQUIRE(result == 2);

    // Runs the clone, which starts from the initial globals rather than the original's
    auto clone = script->clone();
    REQUIRE(clone);
    REQUIRE(clone->run("countCalls", arguments, nullptr, error));
    REQUIRE(result == 1);
}
