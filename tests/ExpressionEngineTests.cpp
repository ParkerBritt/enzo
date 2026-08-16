#include "Engine/Expression/ExpressionEngine.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeTypeTable.h"
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

static nt::NodeType transformNodeType()
{
    nt::NodeTypeTable::initPlugins();
    return nt::NodeTypeTable::getNodeType("transform").value();
}

static nt::NodeType gridNodeType()
{
    nt::NodeTypeTable::initPlugins();
    return nt::NodeTypeTable::getNodeType("grid").value();
}

static nt::NodeType pathNodeType()
{
    nt::NodeTypeTable::initPlugins();
    return nt::NodeTypeTable::getNodeType("path").value();
}

TEST_CASE_METHOD(NMReset, "Prm reads another node's parameter by path")
{
    auto& nm = nt::nm();

    // The source node holds the value the expression should pull
    nt::NodeId source = nm.createNode(transformNodeType());
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    // A second node reads the source's translate through a path expression
    nt::NodeId reader = nm.createNode(transformNodeType());
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform_1.translate\")");

    REQUIRE(translate->evalFloat() == 7.0f);
}

TEST_CASE_METHOD(NMReset, "Prm reads a chosen component of a vector parameter")
{
    auto& nm = nt::nm();

    // The source holds a distinct value in each component of its translate
    nt::NodeId source = nm.createNode(transformNodeType());
    auto sourceTranslate = nm.getNode(source).getParameter("translate").lock();
    sourceTranslate->setFloat(1.0f, 0);
    sourceTranslate->setFloat(2.0f, 1);
    sourceTranslate->setFloat(3.0f, 2);

    // The reader pulls the second and third components by index
    nt::NodeId reader = nm.createNode(transformNodeType());
    auto translate = nm.getNode(reader).getParameter("translate").lock();

    translate->setExpression("prm(\"transform_1.translate\", 1)");
    REQUIRE(translate->evalFloat() == 2.0f);

    translate->setExpression("prm(\"transform_1.translate\", 2)");
    REQUIRE(translate->evalFloat() == 3.0f);
}

TEST_CASE_METHOD(NMReset, "Prm without an index reads the first component")
{
    auto& nm = nt::nm();

    // The source holds different values across its translate components
    nt::NodeId source = nm.createNode(transformNodeType());
    auto sourceTranslate = nm.getNode(source).getParameter("translate").lock();
    sourceTranslate->setFloat(1.0f, 0);
    sourceTranslate->setFloat(2.0f, 1);

    // Omitting the index reads the same component as passing 0
    nt::NodeId reader = nm.createNode(transformNodeType());
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform_1.translate\")");

    REQUIRE(translate->evalFloat() == 1.0f);
}

TEST_CASE_METHOD(NMReset, "Changing a parameter recooks nodes whose expressions read it")
{
    auto& nm = nt::nm();

    // The source node holds the value the reader pulls
    nt::NodeId source = nm.createNode(transformNodeType());
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    // The reader pulls the source's translate through an expression
    nt::NodeId reader = nm.createNode(transformNodeType());
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"transform_1.translate\")");

    // Evaluating once records the captured dependency, then cooking clears the
    // reader so the later source change is what dirties it
    translate->evalFloat();
    nm.cook(reader);
    REQUIRE_FALSE(nm.getNode(reader).isDirty());

    // Changing the source marks the reader stale through the captured edge
    nm.getNode(source).getParameter("translate").lock()->setFloat(9.0f);
    REQUIRE(nm.getNode(reader).isDirty());
}

TEST_CASE_METHOD(NMReset, "Prm reports an error when the path matches no parameter")
{
    auto& nm = nt::nm();

    // The reader points its expression at a node that does not exist
    nt::NodeId reader = nm.createNode(transformNodeType());
    auto translate = nm.getNode(reader).getParameter("translate").lock();
    translate->setExpression("prm(\"does_not_exist.translate\")");

    // A bad path falls back to zero and surfaces the failure as an error
    String error;
    REQUIRE(translate->evalFloat(0, error) == 0.0f);
    REQUIRE_FALSE(error.empty());
}

TEST_CASE_METHOD(NMReset, "PrmI reads another node's integer parameter")
{
    auto& nm = nt::nm();

    // The source node holds the integer the expression should pull
    nt::NodeId source = nm.createNode(gridNodeType());
    nm.getNode(source).getParameter("rows").lock()->setInt(5);

    // A second node reads the source's rows through a path expression
    nt::NodeId reader = nm.createNode(gridNodeType());
    auto rows = nm.getNode(reader).getParameter("rows").lock();
    rows->setExpression("prmI(\"grid_1.rows\")");

    REQUIRE(rows->evalInt() == 5);
}

TEST_CASE_METHOD(NMReset, "PrmS reads another node's string parameter")
{
    auto& nm = nt::nm();

    // The source node holds the string the expression should pull
    nt::NodeId source = nm.createNode(pathNodeType());
    nm.getNode(source).getParameter("path").lock()->setString("hello");

    // A second node reads the source's path through a path expression
    nt::NodeId reader = nm.createNode(pathNodeType());
    auto path = nm.getNode(reader).getParameter("path").lock();
    path->setExpression("prmS(\"path_1.path\")");

    REQUIRE(path->evalString() == "hello");
}
