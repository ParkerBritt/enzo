#include "Engine/Script/PointScript.h"
#include "Engine/Script/ScriptContext.h"
#include "Engine/Script/ScriptEngine.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Primitives/Mesh.h"
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

TEST_CASE("compile reuses the compiled script for the same code")
{
    String error;
    auto first = script::ScriptEngine::instance().compile("@doubled = float(curPt() * 2l)", error);
    auto second = script::ScriptEngine::instance().compile("@doubled = float(curPt() * 2l)", error);
    REQUIRE(first);
    REQUIRE(first == second);
}

TEST_CASE("compile reports code that does not compile")
{
    String error;
    REQUIRE_FALSE(script::ScriptEngine::instance().compile("let broken = ", error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE_METHOD(NMReset, "A script run over many points reads a parameter once")
{
    auto& nm = nt::nm();
    nt::NodeId source = nm.createNode("enzo::transform");
    nm.getNode(source).getParameter("translate").lock()->setFloat(7.0f);

    String error;
    auto script = script::ScriptEngine::instance().compile(R"(@offset = prm("transform1.translate"))", error);
    INFO(error);
    REQUIRE(script);

    auto input = std::make_shared<geo::Mesh>();
    for (int point = 0; point < 100; ++point) input->addPoint(Vector3::Zero());
    NodePacket packet;
    packet.addPrimitive(input);
    geo::Mesh output = *input;

    // Runs the whole point range against one context, as a single cook does
    auto instance = script->clone();
    const script::ScriptContext context(source, packet);
    REQUIRE(instance->addWrittenAttributes(output, error));
    REQUIRE(instance->run(context, 0, output, 0, input->getNumPoints(), error));

    auto offsets = output.getAttribByName(attr::AttributeOwner::POINT, "offset");
    REQUIRE(attr::AttributeHandleRO<floatT>(offsets).getValue(99) == 7.0f);
    REQUIRE(context.getExpressionDependencies().size() == 1);
    REQUIRE(context.getExpressionDependencies()[0] == nt::Unit{source});
}
