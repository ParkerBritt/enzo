#include "Engine/Expression/ExpressionEngine.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("evalFloat evaluates an arithmetic expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    REQUIRE(engine.evalFloat("5 + 5", result, error));
    REQUIRE(result == 10.0f);
}

TEST_CASE("evalInt evaluates an arithmetic expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    intT result = 0;
    String error;
    REQUIRE(engine.evalInt("2 * 3 + 1", result, error));
    REQUIRE(result == 7);
}

TEST_CASE("evalFloat reports an error for a malformed expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    REQUIRE_FALSE(engine.evalFloat("5 +", result, error));
    REQUIRE_FALSE(error.empty());
}
