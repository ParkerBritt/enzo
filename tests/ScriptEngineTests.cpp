#include "Engine/Expression/DasRuntime.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Expression/ScriptEngine.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Parameter/NodeParameter.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

struct NMReset
{
    NMReset()
    {
        nt::NodeLoader::loadNodes();
        nt::nm()._reset();
    }
    ~NMReset() { nt::nm()._reset(); }
};

namespace {
bool runScript(
    expr::CompiledScript& script,
    intT point,
    const expr::ExpressionContext* context,
    String& error
)
{
    const expr::ScriptArgument arguments[] = {point};
    return script.run(expr::ScriptEngine::runFunctionName, arguments, context, error);
}
} // namespace

TEST_CASE("compile runs the user's code with the point offset")
{
    String error;
    auto script = expr::ScriptEngine::instance().compile(
        R"(if (pt == 3l) {
    panic("reached point 3")
})",
        error
    );
    INFO(error);
    REQUIRE(script);

    REQUIRE(runScript(*script, 2, nullptr, error));
    REQUIRE_FALSE(runScript(*script, 3, nullptr, error));
    REQUIRE(error.find("reached point 3") != String::npos);
}

TEST_CASE("compile reuses the compiled script for the same code")
{
    String error;
    auto first = expr::ScriptEngine::instance().compile("let doubled = pt * 2l", error);
    auto second = expr::ScriptEngine::instance().compile("let doubled = pt * 2l", error);
    REQUIRE(first);
    REQUIRE(first == second);
}

TEST_CASE("compile reports code that does not compile")
{
    String error;
    REQUIRE_FALSE(expr::ScriptEngine::instance().compile("let broken = ", error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE_METHOD(NMReset, "A script run over many points reads a parameter once")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    String error;
    auto script = expr::ScriptEngine::instance().compile(
        R"(if (prm("transform1.translate") != 7.0) {
    panic("read the wrong value")
})",
        error
    );
    INFO(error);
    REQUIRE(script);

    // Runs every point against one context, as a single cook does
    expr::ExpressionContext context(source);
    for (intT point = 0; point < 100; ++point)
    {
        REQUIRE(runScript(*script, point, &context, error));
    }

    REQUIRE(context.getExpressionDependencies().size() == 1);
    REQUIRE(context.getExpressionDependencies()[0] == nt::Unit{source});
}
