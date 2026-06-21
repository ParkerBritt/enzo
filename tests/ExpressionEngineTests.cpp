#include "Engine/Expression/ExpressionEngine.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/OperatorTable.h"
#include "Engine/Parameter/NodeParameter.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("evalFloat evaluates an arithmetic expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    REQUIRE(engine.evalFloat("5 + 5", nullptr, result, error));
    REQUIRE(result == 10.0f);
}

TEST_CASE("evalInt evaluates an arithmetic expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    intT result = 0;
    String error;
    REQUIRE(engine.evalInt("2 * 3 + 1", nullptr, result, error));
    REQUIRE(result == 7);
}

TEST_CASE("evalFloat reports an error for a malformed expression")
{
    expr::ExpressionEngine& engine = expr::ExpressionEngine::instance();

    floatT result = 0;
    String error;
    REQUIRE_FALSE(engine.evalFloat("5 +", nullptr, result, error));
    REQUIRE_FALSE(error.empty());
}

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

static op::OpInfo transformOpInfo()
{
    op::OperatorTable::initPlugins();
    return op::OperatorTable::getOpInfo("transform").value();
}

TEST_CASE_METHOD(NMReset, "Prm reads another node's parameter by path")
{
    auto& nm = nt::nm();

    // The source node holds the value the expression should pull
    nt::OpId source = nm.createOperator(transformOpInfo());
    nm.getGeoOperator(source).getParameter("translate").lock()->setFloat(7.0f);

    // A second node reads the source's translate through a path expression
    nt::OpId reader = nm.createOperator(transformOpInfo());
    auto translate = nm.getGeoOperator(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform_1.translate\")");

    REQUIRE(translate->evalFloat() == 7.0f);
}

TEST_CASE_METHOD(NMReset, "Changing a parameter recooks nodes whose expressions read it")
{
    auto& nm = nt::nm();

    // The source node holds the value the reader pulls
    nt::OpId source = nm.createOperator(transformOpInfo());
    nm.getGeoOperator(source).getParameter("translate").lock()->setFloat(7.0f);

    // The reader pulls the source's translate through an expression
    nt::OpId reader = nm.createOperator(transformOpInfo());
    auto translate = nm.getGeoOperator(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform_1.translate\")");

    // Evaluating once records the captured dependency, then cooking clears the
    // reader so the later source change is what dirties it
    translate->evalFloat();
    nm.cookOp(reader);
    REQUIRE_FALSE(nm.getGeoOperator(reader).isDirty());

    // Changing the source marks the reader stale through the captured edge
    nm.getGeoOperator(source).getParameter("translate").lock()->setFloat(9.0f);
    REQUIRE(nm.getGeoOperator(reader).isDirty());
}
