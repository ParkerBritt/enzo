#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Primitive.h"
#include <Engine/Primitives/Mesh.h>
#include <Engine/Selection/IndexSet.h>
#include <Engine/Selection/Selection.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace enzo;
TEST_CASE("getPrims")
{
    // Setup packet
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(unselectedPrim);
    packet.addPrimitive(selectedPrim);

    Selection selection("/selected");
    auto primitives = selection.getPrims(packet);
    REQUIRE(primitives.size() == 1);
    REQUIRE(primitives[0]->getPath() == "/selected");
}

TEST_CASE("containsPrim")
{
    // Setup packet
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr partiallySelectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(unselectedPrim);
    packet.addPrimitive(selectedPrim);

    Selection selection("/selected f{10}");
    REQUIRE(selection.containsPrim(selectedPrim));
    REQUIRE(selection.containsPrim(partiallySelectedPrim));
    REQUIRE_FALSE(selection.containsPrim(unselectedPrim));
}

TEST_CASE("Selection containsFace for explicit face indices")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{10}");
    REQUIRE(selection.containsFace(prim, 10, 10));
    REQUIRE_FALSE(selection.containsFace(prim, 11, 11));
}

TEST_CASE("Selection getFaces explicit indices")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    for (int i = 0; i < 11; ++i)
    {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected f{10}");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<Index>{10});
}

TEST_CASE("Selection containsFace wildcard")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{*}");
    REQUIRE(selection.containsFace(prim, 0, 0));
    REQUIRE(selection.containsFace(prim, 999, 999));
}

TEST_CASE("Selection wildcard does not apply to unselected prims")
{
    geo::PrimPtr other = std::make_shared<geo::Mesh>("/other");
    Selection selection("/selected f{*}");
    REQUIRE_FALSE(selection.containsFace(other, 0, 0));
}

TEST_CASE("Selection getFaces wildcard")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    for (int i = 0; i < 5; ++i)
    {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected f{*}");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<Index>{0, 1, 2, 3, 4});
}

TEST_CASE("Selection empty string selects every prim")
{
    NodePacket packet;
    geo::PrimPtr a = std::make_shared<geo::Mesh>("/a");
    geo::PrimPtr b = std::make_shared<geo::Mesh>("/b");
    packet.addPrimitive(a);
    packet.addPrimitive(b);

    Selection selection("");
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 2);
    // Both prims are matched, and as whole-prim (no blocks present).
    REQUIRE(selection.containsPrim(a));
    REQUIRE(selection.containsPrim(a, true));
    REQUIRE(selection.containsPrim(b, true));
}

TEST_CASE("Selection empty string contains every component")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/foo");
    Selection selection("");

    // Whole-prim semantics extend to faces, points, and vertices.
    REQUIRE(selection.containsPoint(prim, 0, 0));
    REQUIRE(selection.containsPoint(prim, 999, 999));
    REQUIRE(selection.containsFace(prim, 0, 0));
    REQUIRE(selection.containsFace(prim, 999, 999));
    REQUIRE(selection.containsVertex(prim, 0, 0));
    REQUIRE(selection.containsVertex(prim, 999, 999));
}

TEST_CASE("Selection empty string getPoints returns all points")
{
    auto mesh = std::make_shared<geo::Mesh>("/foo");
    for (int i = 0; i < 4; ++i)
    {
        mesh->addPoint(Vector3(i, 0, 0));
    }
    Selection selection("");
    auto points = selection.getPoints(mesh);
    REQUIRE(points == std::vector<Offset>{0, 1, 2, 3});
}

TEST_CASE("Selection empty string getFaces returns all faces")
{
    auto mesh = std::make_shared<geo::Mesh>("/foo");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    for (int i = 0; i < 3; ++i)
    {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<Offset>{0, 1, 2});
}

TEST_CASE("Selection empty string inverted selects nothing")
{
    NodePacket packet;
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/foo");
    packet.addPrimitive(prim);

    Selection selection("");
    selection.setInverted(true);

    // The complement of "everything" is "nothing".
    REQUIRE(selection.getPrims(packet).empty());
    REQUIRE_FALSE(selection.containsPrim(prim));
    REQUIRE_FALSE(selection.containsFace(prim, 0, 0));
    REQUIRE_FALSE(selection.containsPoint(prim, 0, 0));
    REQUIRE_FALSE(selection.containsVertex(prim, 0, 0));
}

TEST_CASE("Selection without path getPrims returns all prims")
{
    NodePacket packet;
    geo::PrimPtr a = std::make_shared<geo::Mesh>("/a");
    geo::PrimPtr b = std::make_shared<geo::Mesh>("/b");
    packet.addPrimitive(a);
    packet.addPrimitive(b);

    Selection selection("p{0}");
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 2);
}

TEST_CASE("Selection without path containsPoint matches on any prim")
{
    geo::PrimPtr a = std::make_shared<geo::Mesh>("/a");
    geo::PrimPtr b = std::make_shared<geo::Mesh>("/b");

    Selection selection("p{0}");
    REQUIRE(selection.containsPoint(a, 0, 0));
    REQUIRE(selection.containsPoint(b, 0, 0));
    REQUIRE_FALSE(selection.containsPoint(a, 1, 1));
}

TEST_CASE("Selection without path containsPrim is partial")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/foo");

    Selection selection("p{0}");
    REQUIRE(selection.containsPrim(prim));
    REQUIRE_FALSE(selection.containsPrim(prim, true));
}

TEST_CASE("setInverted toggles the flag")
{
    Selection selection("/a");
    REQUIRE_FALSE(selection.getInverted());
    selection.setInverted(true);
    REQUIRE(selection.getInverted());
}

TEST_CASE("containsPrim inverted")
{
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");

    // Whole-prim selection: inverted hides the named prim and exposes others
    Selection whole("/selected");
    whole.setInverted(true);
    REQUIRE_FALSE(whole.containsPrim(selectedPrim));
    REQUIRE(whole.containsPrim(unselectedPrim));
    REQUIRE(whole.containsPrim(unselectedPrim, true));

    // Partial selection: inverted still touches the named prim, but not as a whole
    Selection partial("/selected f{0}");
    partial.setInverted(true);
    REQUIRE(partial.containsPrim(selectedPrim));
    REQUIRE_FALSE(partial.containsPrim(selectedPrim, true));
    REQUIRE(partial.containsPrim(unselectedPrim, true));
}

TEST_CASE("containsFace inverted")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{10}");
    selection.setInverted(true);
    REQUIRE_FALSE(selection.containsFace(prim, 10, 10));
    REQUIRE(selection.containsFace(prim, 11, 11));
}

TEST_CASE("getPrims inverted includes unmentioned prims")
{
    NodePacket packet;
    geo::PrimPtr selectedPrim = std::make_shared<geo::Mesh>("/selected");
    geo::PrimPtr unselectedPrim = std::make_shared<geo::Mesh>("/unselected");
    packet.addPrimitive(selectedPrim);
    packet.addPrimitive(unselectedPrim);

    Selection selection("/selected");
    selection.setInverted(true);
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 1);
    REQUIRE(prims[0]->getPath() == "/unselected");
}

TEST_CASE("Selection containsPoint range")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected p{0-8}");

    // Endpoints and interior are included
    REQUIRE(selection.containsPoint(prim, 0, 0));
    REQUIRE(selection.containsPoint(prim, 4, 4));
    REQUIRE(selection.containsPoint(prim, 8, 8));

    // Outside the range is excluded
    REQUIRE_FALSE(selection.containsPoint(prim, 9, 9));
}

TEST_CASE("Selection containsFace range")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{2-4}");
    REQUIRE_FALSE(selection.containsFace(prim, 1, 1));
    REQUIRE(selection.containsFace(prim, 2, 2));
    REQUIRE(selection.containsFace(prim, 3, 3));
    REQUIRE(selection.containsFace(prim, 4, 4));
    REQUIRE_FALSE(selection.containsFace(prim, 5, 5));
}

TEST_CASE("Selection range mixed with explicit indices")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected p{0-2 5 7-8}");

    // From the range 0-2
    REQUIRE(selection.containsPoint(prim, 0, 0));
    REQUIRE(selection.containsPoint(prim, 2, 2));
    // Gap between range and explicit value
    REQUIRE_FALSE(selection.containsPoint(prim, 3, 3));
    REQUIRE_FALSE(selection.containsPoint(prim, 4, 4));
    // Explicit single index
    REQUIRE(selection.containsPoint(prim, 5, 5));
    REQUIRE_FALSE(selection.containsPoint(prim, 6, 6));
    // From the range 7-8
    REQUIRE(selection.containsPoint(prim, 7, 7));
    REQUIRE(selection.containsPoint(prim, 8, 8));
    REQUIRE_FALSE(selection.containsPoint(prim, 9, 9));
}

TEST_CASE("Selection range single-element treats endpoints equally")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected p{3-3}");
    REQUIRE(selection.containsPoint(prim, 3, 3));
    REQUIRE_FALSE(selection.containsPoint(prim, 2, 2));
    REQUIRE_FALSE(selection.containsPoint(prim, 4, 4));
}

TEST_CASE("Selection getPoints range walks valid points")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    // Build 10 points so we can range across them
    for (int i = 0; i < 10; ++i)
    {
        mesh->addPoint(Vector3(i, 0, 0));
    }
    Selection selection("/selected p{2-5}");
    auto points = selection.getPoints(mesh);
    REQUIRE(points == std::vector<Offset>{2, 3, 4, 5});
}

TEST_CASE("getFaces inverted returns complement")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    for (int i = 0; i < 5; ++i)
    {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected f{2}");
    selection.setInverted(true);
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<Offset>{0, 1, 3, 4});
}

// Inverting a partial selection only flips membership for the primitive part
// it actually talks about. A face-only selection inverted is still face-only.

TEST_CASE("Inverted face-only selection leaves points untouched")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/mesh");
    Selection selection("/mesh f{0-20}");
    selection.setInverted(true);

    // Faces flip within the face set
    REQUIRE_FALSE(selection.containsFace(prim, 0, 0));
    REQUIRE_FALSE(selection.containsFace(prim, 20, 20));
    REQUIRE(selection.containsFace(prim, 21, 21));

    // Points and vertices were never part of the selection, so the inversion
    // shouldn't drag them in either.
    REQUIRE_FALSE(selection.containsPoint(prim, 0, 0));
    REQUIRE_FALSE(selection.containsPoint(prim, 99, 99));
    REQUIRE_FALSE(selection.containsVertex(prim, 0, 0));
    REQUIRE_FALSE(selection.containsVertex(prim, 99, 99));
}

TEST_CASE("Inverted face-only getPoints and getVertices are empty")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    for (int i = 0; i < 5; ++i)
    {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/mesh f{0}");
    selection.setInverted(true);

    REQUIRE(selection.getPoints(mesh).empty());
    REQUIRE(selection.getVertices(mesh).empty());
    // The face complement is still there.
    REQUIRE(selection.getFaces(mesh) == std::vector<Offset>{1, 2, 3, 4});
}

TEST_CASE("Inverted point-only selection leaves faces and vertices untouched")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/mesh");
    Selection selection("/mesh p{0-2}");
    selection.setInverted(true);

    REQUIRE_FALSE(selection.containsPoint(prim, 0, 0));
    REQUIRE_FALSE(selection.containsPoint(prim, 2, 2));
    REQUIRE(selection.containsPoint(prim, 3, 3));

    REQUIRE_FALSE(selection.containsFace(prim, 0, 0));
    REQUIRE_FALSE(selection.containsVertex(prim, 0, 0));
}

TEST_CASE("Inverted vertex-only selection leaves faces and points untouched")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/mesh");
    Selection selection("/mesh v{0-2}");
    selection.setInverted(true);

    REQUIRE_FALSE(selection.containsVertex(prim, 0, 0));
    REQUIRE_FALSE(selection.containsVertex(prim, 2, 2));
    REQUIRE(selection.containsVertex(prim, 3, 3));

    REQUIRE_FALSE(selection.containsFace(prim, 0, 0));
    REQUIRE_FALSE(selection.containsPoint(prim, 0, 0));
}

TEST_CASE("Inverted pathless face selection inverts faces on every prim")
{
    geo::PrimPtr a = std::make_shared<geo::Mesh>("/a");
    geo::PrimPtr b = std::make_shared<geo::Mesh>("/b");
    Selection selection("f{0-2}");
    selection.setInverted(true);

    // The face complement applies on each prim, but no points or vertices are pulled in.
    REQUIRE_FALSE(selection.containsFace(a, 1, 1));
    REQUIRE(selection.containsFace(a, 3, 3));
    REQUIRE_FALSE(selection.containsFace(b, 1, 1));
    REQUIRE(selection.containsFace(b, 3, 3));

    REQUIRE_FALSE(selection.containsPoint(a, 0, 0));
    REQUIRE_FALSE(selection.containsVertex(b, 0, 0));
}

// Whole-prim selection: no blocks present, so every component is implicitly included.

TEST_CASE("containsPoint whole-prim")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected");
    REQUIRE(selection.containsPoint(prim, 0, 0));
    REQUIRE(selection.containsPoint(prim, 42, 42));
}

TEST_CASE("containsFace whole-prim")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected");
    REQUIRE(selection.containsFace(prim, 0, 0));
    REQUIRE(selection.containsFace(prim, 42, 42));
}

TEST_CASE("containsVertex whole-prim")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected");
    REQUIRE(selection.containsVertex(prim, 0, 0));
    REQUIRE(selection.containsVertex(prim, 42, 42));
}

TEST_CASE("getPoints whole-prim")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    for (int i = 0; i < 5; ++i)
    {
        mesh->addPoint(Vector3(i, 0, 0));
    }
    Selection selection("/selected");
    auto points = selection.getPoints(mesh);
    REQUIRE(points == std::vector<Offset>{0, 1, 2, 3, 4});
}

TEST_CASE("getFaces whole-prim")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    for (int i = 0; i < 3; ++i)
    {
        mesh->addFace({p0, p1, p2});
    }
    Selection selection("/selected");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<Offset>{0, 1, 2});
}

TEST_CASE("getVertices whole-prim")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({p0, p1, p2});
    Selection selection("/selected");
    auto verts = selection.getVertices(mesh);
    REQUIRE(verts.size() == 3);
}

// Partial selection: any block present restricts the selection to only those listed blocks.

TEST_CASE("containsFace excluded when only points listed")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected p{0}");
    REQUIRE_FALSE(selection.containsFace(prim, 0, 0));
}

TEST_CASE("containsVertex excluded when only points listed")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected p{0}");
    REQUIRE_FALSE(selection.containsVertex(prim, 0, 0));
}

TEST_CASE("containsPoint excluded when only faces listed")
{
    geo::PrimPtr prim = std::make_shared<geo::Mesh>("/selected");
    Selection selection("/selected f{0}");
    REQUIRE_FALSE(selection.containsPoint(prim, 0, 0));
}

TEST_CASE("getFaces empty when only points listed")
{
    auto mesh = std::make_shared<geo::Mesh>("/selected");
    Offset p0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset p1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset p2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({p0, p1, p2});
    Selection selection("/selected p{0}");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces.empty());
}

// Group selections: a name without a leading slash names a group rather than a path.

TEST_CASE("Selection by group name returns faces in the group")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");

    // Build three faces, mark 0 and 2 as members of the group
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->createFaceGroup("highlighted");
    mesh->addToFaceGroup("highlighted", {0, 2});

    Selection selection("highlighted");
    auto faces = selection.getFaces(mesh);
    REQUIRE(faces == std::vector<Offset>{0, 2});
}

TEST_CASE("Two group components combine into the union of their faces")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->createFaceGroup("groupA");
    mesh->createFaceGroup("groupB");
    mesh->addToFaceGroup("groupA", {0});
    mesh->addToFaceGroup("groupB", {2});

    // Two comma separated group components should select the union.
    Selection selection("groupA,groupB");
    REQUIRE(selection.getFaces(mesh) == std::vector<Offset>{0, 2});
}

TEST_CASE("Inverting two group components selects the complement of their union")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->createFaceGroup("groupA");
    mesh->createFaceGroup("groupB");
    mesh->addToFaceGroup("groupA", {0});
    mesh->addToFaceGroup("groupB", {2});

    // Inverting must exclude every face in either group, not everything but
    // their intersection. Faces 1 and 3 are in neither group.
    Selection selection("groupA,groupB");
    selection.setInverted(true);
    REQUIRE(selection.getFaces(mesh) == std::vector<Offset>{1, 3});
}

TEST_CASE("Selection by group name uses offsets not compacted indices")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");

    // Four faces, then delete the second so storage offsets and compacted
    // indices no longer line up: valid offsets {0, 2, 3} map to indices {0, 1, 2}.
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->deleteFaces({1});

    // The group member lives at offset 3, which is compacted index 2.
    mesh->createFaceGroup("highlighted");
    mesh->addToFaceGroup("highlighted", {3});

    // Group membership must be read by offset, so only offset 3 is selected.
    Selection selection("highlighted");
    REQUIRE(selection.getFaces(mesh) == std::vector<Offset>{3});
}

TEST_CASE("Inverted face group selects no points or vertices")
{
    // Mirrors an extrude feeding a delete: a face-only group, then inverted.
    // The group says nothing about points or vertices, so inverting it must
    // not sweep them all in (which would delete the whole mesh downstream).
    auto mesh = std::make_shared<geo::Mesh>("/mesh");
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->createFaceGroup("extrudeBottom");
    mesh->addToFaceGroup("extrudeBottom", {0});

    Selection selection("extrudeBottom");
    selection.setInverted(true);

    // Faces: everything except the group member.
    REQUIRE(selection.getFaces(mesh) == std::vector<Offset>{1});
    // Points and vertices: the group does not talk about them, so none.
    REQUIRE(selection.getPoints(mesh).empty());
    REQUIRE(selection.getVertices(mesh).empty());
}

TEST_CASE("Inverted group selection excludes only the group members on a fragmented mesh")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");

    // Same fragmented layout: valid offsets {0, 2, 3}, with the group at offset 3.
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->addFace({point0, point1, point2});
    mesh->deleteFaces({1});
    mesh->createFaceGroup("highlighted");
    mesh->addToFaceGroup("highlighted", {3});

    // Inverting selects every valid face except the group member, not everything.
    Selection selection("highlighted");
    selection.setInverted(true);
    REQUIRE(selection.getFaces(mesh) == std::vector<Offset>{0, 2});
}

TEST_CASE("Selection by group name on a prim without the group returns empty")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");

    // Build a face so the mesh isn't empty
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});

    Selection selection("missingGroup");
    REQUIRE(selection.getFaces(mesh).empty());
    REQUIRE_FALSE(selection.containsPrim(mesh));
}

TEST_CASE("Selection by group name getPrims only includes prims with the group")
{
    NodePacket packet;
    auto withGroup = std::make_shared<geo::Mesh>("/with");
    auto withoutGroup = std::make_shared<geo::Mesh>("/without");
    withGroup->createFaceGroup("g");
    packet.addPrimitive(withGroup);
    packet.addPrimitive(withoutGroup);

    Selection selection("g");
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 1);
    REQUIRE(prims[0]->getPath() == "/with");
}

TEST_CASE("Selection by primitive group includes prims where the flag is set")
{
    NodePacket packet;
    auto active = std::make_shared<geo::Mesh>("/active");
    auto inactive = std::make_shared<geo::Mesh>("/inactive");

    // Both prims have the group, but only one has the flag set
    active->createPrimitiveGroup("active");
    active->addToPrimitiveGroup("active", {0});
    inactive->createPrimitiveGroup("active");

    packet.addPrimitive(active);
    packet.addPrimitive(inactive);

    Selection selection("active");
    auto prims = selection.getPrims(packet);
    REQUIRE(prims.size() == 1);
    REQUIRE(prims[0]->getPath() == "/active");
}

TEST_CASE("Selection by primitive group treats the prim as whole-prim selection")
{
    auto mesh = std::make_shared<geo::Mesh>("/mesh");
    Offset point0 = mesh->addPoint(Vector3(0, 0, 0));
    Offset point1 = mesh->addPoint(Vector3(1, 0, 0));
    Offset point2 = mesh->addPoint(Vector3(0, 1, 0));
    mesh->addFace({point0, point1, point2});
    mesh->createPrimitiveGroup("active");
    mesh->addToPrimitiveGroup("active", {0});

    Selection selection("active");

    // Whole prim is in, so every element type is too
    REQUIRE(selection.containsPrim(mesh, true));
    REQUIRE(selection.containsFace(mesh, 0, 0));
    REQUIRE(selection.containsPoint(mesh, 0, 0));
    REQUIRE(selection.containsVertex(mesh, 0, 0));
}
