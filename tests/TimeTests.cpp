#include "Engine/Core/Types.h"
#include "Engine/Network/CookContext.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/NetworkGraph/NetworkGraph.h"
#include "Engine/Parameter/NodeParameter.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

struct NMReset
{
    NMReset() { enzo::nt::nm()._reset(); }
    ~NMReset() { enzo::nt::nm()._reset(); }
};

TEST_CASE_METHOD(NMReset, "the scene opens on frame one of a twenty four frames per second range")
{
    auto& nm = enzo::nt::nm();

    REQUIRE(nm.getFrame() == 1);
    REQUIRE(nm.getStartFrame() == 1);
    REQUIRE(nm.getEndFrame() == 240);
    REQUIRE(nm.getFps() == 24);
}

TEST_CASE_METHOD(NMReset, "the frame holds whatever it is set to inside the range")
{
    auto& nm = enzo::nt::nm();

    nm.setFrame(72);
    REQUIRE(nm.getFrame() == 72);

    // A frame between two frames is a valid place to sit.
    nm.setFrame(72.5f);
    REQUIRE(nm.getFrame() == Catch::Approx(72.5f));
}

TEST_CASE_METHOD(NMReset, "a frame outside the range clamps to the nearest end of it")
{
    auto& nm = enzo::nt::nm();

    nm.setFrame(-10);
    REQUIRE(nm.getFrame() == 1);

    nm.setFrame(1000);
    REQUIRE(nm.getFrame() == 240);
}

TEST_CASE_METHOD(NMReset, "a start frame past the end carries the end along with it")
{
    auto& nm = enzo::nt::nm();

    nm.setStartFrame(300);

    REQUIRE(nm.getStartFrame() == 300);
    REQUIRE(nm.getEndFrame() == 300);
}

TEST_CASE_METHOD(NMReset, "an end frame before the start carries the start along with it")
{
    auto& nm = enzo::nt::nm();

    nm.setStartFrame(100);
    nm.setEndFrame(50);

    REQUIRE(nm.getStartFrame() == 50);
    REQUIRE(nm.getEndFrame() == 50);
}

TEST_CASE_METHOD(NMReset, "narrowing the range pulls the frame back inside it")
{
    auto& nm = enzo::nt::nm();

    nm.setFrame(200);
    nm.setEndFrame(100);
    REQUIRE(nm.getFrame() == 100);

    nm.setStartFrame(50);
    REQUIRE(nm.getFrame() == 100);

    nm.setStartFrame(150);
    REQUIRE(nm.getFrame() == 150);
}

TEST_CASE_METHOD(NMReset, "the time in seconds counts from the start of frame one")
{
    auto& nm = enzo::nt::nm();

    REQUIRE(nm.getTime() == 0);

    nm.setFrame(25);
    REQUIRE(nm.getTime() == Catch::Approx(1.f));

    nm.setFps(48);
    REQUIRE(nm.getTime() == Catch::Approx(0.5f));
}

TEST_CASE_METHOD(NMReset, "a playback rate of zero or less is refused")
{
    auto& nm = enzo::nt::nm();

    nm.setFps(0);
    REQUIRE(nm.getFps() == 24);

    nm.setFps(-30);
    REQUIRE(nm.getFps() == 24);
}

TEST_CASE_METHOD(NMReset, "the frame signal reports every move and nothing else")
{
    auto& nm = enzo::nt::nm();

    unsigned int moveCount = 0;
    enzo::floatT lastFrame = 0;
    auto connection = nm.frameChanged.connect([&](enzo::floatT frame) {
        ++moveCount;
        lastFrame = frame;
    });

    nm.setFrame(10);
    REQUIRE(moveCount == 1);
    REQUIRE(lastFrame == 10);

    nm.setFrame(10);
    REQUIRE(moveCount == 1);

    // Clamping still lands on a new frame, so it counts as a move.
    nm.setFrame(1000);
    REQUIRE(moveCount == 2);
    REQUIRE(lastFrame == 240);

    connection.disconnect();
}

TEST_CASE_METHOD(NMReset, "clearing the scene puts the time back to its defaults")
{
    auto& nm = enzo::nt::nm();

    nm.setStartFrame(10);
    nm.setEndFrame(50);
    nm.setFrame(30);
    nm.setFps(30);

    nm.clear();

    REQUIRE(nm.getFrame() == 1);
    REQUIRE(nm.getStartFrame() == 1);
    REQUIRE(nm.getEndFrame() == 240);
    REQUIRE(nm.getFps() == 24);
}

TEST_CASE_METHOD(NMReset, "a cook reads the frame the scene sits on")
{
    enzo::nt::NodeLoader::loadNodes();
    auto& nm = enzo::nt::nm();

    enzo::nt::NodeId nodeId = nm.createNode(enzo::nt::NodeTypeTable::requireNodeType("enzo::grid"));
    enzo::nt::CookContext context(nodeId, nm);

    nm.setFrame(49);

    REQUIRE(context.getFrame() == 49);
    REQUIRE(context.getTime() == Catch::Approx(2.f));
}

TEST_CASE_METHOD(NMReset, "moving the frame dirties a parameter whose expression reads it")
{
    enzo::nt::NodeLoader::loadNodes();
    auto& nm = enzo::nt::nm();

    enzo::nt::NodeId nodeId =
        nm.createNode(enzo::nt::NodeTypeTable::requireNodeType("enzo::transform"));
    auto translate = nm.getNode(nodeId).getParameter("translate").lock();
    translate->setExpression("frame()");

    // Records the read and leaves the node clean, so only the frame move can dirty it
    translate->evalFloat();
    nm.cook(nodeId);
    REQUIRE_FALSE(nm.getNode(nodeId).isDirty());

    nm.setFrame(20);
    REQUIRE(nm.getNode(nodeId).isDirty());
    REQUIRE(translate->evalFloat() == 20.0f);
}

TEST_CASE_METHOD(NMReset, "moving the frame leaves a node that never read it alone")
{
    enzo::nt::NodeLoader::loadNodes();
    auto& nm = enzo::nt::nm();

    enzo::nt::NodeId nodeId = nm.createNode(enzo::nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nm.cook(nodeId);
    REQUIRE_FALSE(nm.getNode(nodeId).isDirty());

    nm.setFrame(20);
    REQUIRE_FALSE(nm.getNode(nodeId).isDirty());
}

TEST_CASE_METHOD(NMReset, "moving the frame dirties a node whose cook read it")
{
    enzo::nt::NodeLoader::loadNodes();
    auto& nm = enzo::nt::nm();

    enzo::nt::NodeId nodeId = nm.createNode(enzo::nt::NodeTypeTable::requireNodeType("enzo::grid"));
    nm.cook(nodeId);

    enzo::nt::CookContext context(nodeId, nm);
    context.getFrame();

    nm.setFrame(20);
    REQUIRE(nm.getNode(nodeId).isDirty());
}

TEST_CASE_METHOD(NMReset, "a cook that no longer reads the frame stops being time dependent")
{
    enzo::nt::NodeLoader::loadNodes();
    auto& nm = enzo::nt::nm();

    enzo::nt::NodeId nodeId = nm.createNode(enzo::nt::NodeTypeTable::requireNodeType("enzo::grid"));
    enzo::nt::CookContext context(nodeId, nm);
    context.getFrame();

    // Clears the read, since the grid's own cook reads no time
    nm.cook(nodeId);

    REQUIRE(nm.graph().getTimeDependents().empty());
}
