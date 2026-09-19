#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Parameter/NodeParameter.h"
#include <catch2/catch_test_macros.hpp>
#include <tbb/parallel_for.h>

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

TEST_CASE_METHOD(NMReset, "readParameter keeps the first value for the rest of the evaluation")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");
    auto translate = nm.getNode(source).getParameter("translate").lock();
    translate->setFloat(7.0f);

    expr::ExpressionContext context(source);
    REQUIRE(context.readParameter<floatT>("transform1.translate", 0) == 7.0f);

    // Changes the parameter mid evaluation and expects the value already read
    translate->setFloat(9.0f);
    REQUIRE(context.readParameter<floatT>("transform1.translate", 0) == 7.0f);
}

TEST_CASE_METHOD(NMReset, "readParameter records a parameter's node once however often it is read")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");

    expr::ExpressionContext context(source);
    context.readParameter<floatT>("transform1.translate", 0);
    context.readParameter<floatT>("transform1.translate", 0);

    REQUIRE(context.getExpressionDependencies().size() == 1);
    REQUIRE(context.getExpressionDependencies()[0] == nt::Unit{source});
}

TEST_CASE_METHOD(NMReset, "readParameter returns nothing when the path matches no parameter")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");

    expr::ExpressionContext context(source);
    REQUIRE_FALSE(context.readParameter<floatT>("does_not_exist.translate", 0));
    REQUIRE(context.getExpressionDependencies().empty());
}

TEST_CASE_METHOD(NMReset, "readParameter gives every thread the same value")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    expr::ExpressionContext context(source);
    std::atomic<int> wrongReads = 0;
    tbb::parallel_for(0, 10000, [&](int) {
        if (context.readParameter<floatT>("transform1.translate", 0) != 7.0f) ++wrongReads;
    });

    REQUIRE(wrongReads == 0);
    REQUIRE(context.getExpressionDependencies().size() == 1);
}
