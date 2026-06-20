#include <Engine/Attribute/AttributeHandle.h>
#include <Engine/Core/Types.h>
#include <Engine/Primitives/Mesh.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace enzo;

TEST_CASE("Delete faces")
{
    geo::Mesh mesh;
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    mesh.deleteFaces({1});

    REQUIRE(mesh.isValidFace(0));
    REQUIRE_FALSE(mesh.isValidFace(1));
    REQUIRE(mesh.isValidFace(2));

    REQUIRE(mesh.isValidVertex(2));
    REQUIRE_FALSE(mesh.isValidVertex(3));
    REQUIRE_FALSE(mesh.isValidVertex(4));
    REQUIRE_FALSE(mesh.isValidVertex(5));
    REQUIRE(mesh.isValidVertex(6));
}

TEST_CASE("Delete points")
{
    geo::Mesh mesh;

    // Create points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 1, 0));

    // Create faces
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset1, pointOffset2, pointOffset3});

    // Delete point 0
    mesh.deletePoints({pointOffset0});

    // Test point 0 is invalid, others still valid
    REQUIRE_FALSE(mesh.isValidPoint(0));
    REQUIRE(mesh.isValidPoint(1));
    REQUIRE(mesh.isValidPoint(2));
    REQUIRE(mesh.isValidPoint(3));

    // Test faces are still valid (point was just detached from face 0)
    REQUIRE(mesh.isValidFace(0));
    REQUIRE(mesh.isValidFace(1));

    // Test only the vertex that referenced point 0 went invalid
    REQUIRE_FALSE(mesh.isValidVertex(0));
    REQUIRE(mesh.isValidVertex(1));
    REQUIRE(mesh.isValidVertex(2));
    REQUIRE(mesh.isValidVertex(3));
    REQUIRE(mesh.isValidVertex(4));
    REQUIRE(mesh.isValidVertex(5));
}

TEST_CASE("Delete vertices")
{
    geo::Mesh mesh;

    // Create points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));

    // Create face
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    // Delete one vertex from the face
    mesh.deleteVertices({1});

    // Test the targeted vertex went invalid, others stayed valid
    REQUIRE(mesh.isValidVertex(0));
    REQUIRE_FALSE(mesh.isValidVertex(1));
    REQUIRE(mesh.isValidVertex(2));

    // Test the face stayed (still has vertices)
    REQUIRE(mesh.isValidFace(0));

    // Test points themselves are untouched
    REQUIRE(mesh.isValidPoint(0));
    REQUIRE(mesh.isValidPoint(1));
    REQUIRE(mesh.isValidPoint(2));
}

TEST_CASE("Delete face adds points to solo")
{
    geo::Mesh mesh;

    // Create points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));

    // Create face using all three points
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    // Test no points are solo (the face references all of them)
    REQUIRE(mesh.getNumSoloPoints() == 0);

    // Delete the face but keep the points alive
    mesh.deleteFaces({0}, false);

    // Test all three points became solo (no face references them now)
    REQUIRE(mesh.getNumSoloPoints() == 3);
}

TEST_CASE("Delete all vertices on face")
{
    geo::Mesh mesh;

    // Create points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));

    // Create face
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    // Delete every vertex on the face
    mesh.deleteVertices({0, 1, 2});

    // Test all vertices went invalid
    REQUIRE_FALSE(mesh.isValidVertex(0));
    REQUIRE_FALSE(mesh.isValidVertex(1));
    REQUIRE_FALSE(mesh.isValidVertex(2));

    // Test the face went away (has no vertices left)
    REQUIRE_FALSE(mesh.isValidFace(0));

    // Test points themselves are untouched
    REQUIRE(mesh.isValidPoint(0));
    REQUIRE(mesh.isValidPoint(1));
    REQUIRE(mesh.isValidPoint(2));
}

TEST_CASE("Delete points and faces")
{
    geo::Mesh mesh;

    // Create points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 1, 0));

    // Create faces
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset1, pointOffset2, pointOffset3});

    // Delete point 0 and any face that uses it
    mesh.deletePoints({pointOffset0}, true);

    // Test point 0 is invalid, others still valid
    REQUIRE_FALSE(mesh.isValidPoint(0));
    REQUIRE(mesh.isValidPoint(1));
    REQUIRE(mesh.isValidPoint(2));
    REQUIRE(mesh.isValidPoint(3));

    // Test face 0 went away with the point, face 1 is untouched
    REQUIRE_FALSE(mesh.isValidFace(0));
    REQUIRE(mesh.isValidFace(1));

    // Test all of face 0's vertices went invalid, face 1's didn't
    REQUIRE_FALSE(mesh.isValidVertex(0));
    REQUIRE_FALSE(mesh.isValidVertex(1));
    REQUIRE_FALSE(mesh.isValidVertex(2));
    REQUIRE(mesh.isValidVertex(3));
    REQUIRE(mesh.isValidVertex(4));
    REQUIRE(mesh.isValidVertex(5));
}

TEST_CASE("Create face group")
{
    geo::Mesh mesh;

    // Build a mesh with two faces
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    mesh.createGroup(attr::AttrOwner::FACE, "myGroup");

    // The group lookup should find it, and the attribute lookup should not
    auto group = mesh.getGroupByName(attr::AttrOwner::FACE, "myGroup");
    REQUIRE(group != nullptr);
    REQUIRE(mesh.getAttribByName(attr::AttrOwner::FACE, "myGroup") == nullptr);

    // Every existing face starts outside the group
    attr::AttributeHandleBool handle(group);
    REQUIRE(handle.getSize() == 2);
    REQUIRE(handle.getValue(0) == false);
    REQUIRE(handle.getValue(1) == false);
}

TEST_CASE("Add to face group")
{
    geo::Mesh mesh;

    // Build a mesh with three faces
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    mesh.createGroup(attr::AttrOwner::FACE, "g");
    mesh.addToGroup(attr::AttrOwner::FACE, "g", {0, 2});

    // Faces 0 and 2 should be members, face 1 should not
    attr::AttributeHandleBool handle(mesh.getGroupByName(attr::AttrOwner::FACE, "g"));
    REQUIRE(handle.getValue(0) == true);
    REQUIRE(handle.getValue(1) == false);
    REQUIRE(handle.getValue(2) == true);
}

TEST_CASE("Create primitive group")
{
    geo::Mesh mesh;

    // Primitive groups have exactly one slot per primitive
    mesh.createGroup(attr::AttrOwner::PRIMITIVE, "selected");

    auto group = mesh.getGroupByName(attr::AttrOwner::PRIMITIVE, "selected");
    REQUIRE(group != nullptr);
    attr::AttributeHandleBool handle(group);
    REQUIRE(handle.getSize() == 1);
    REQUIRE(handle.getValue(0) == false);

    // Toggle this primitive into the group
    mesh.addToGroup(attr::AttrOwner::PRIMITIVE, "selected", {0});
    REQUIRE(handle.getValue(0) == true);
}

TEST_CASE("Group name can match attribute name")
{
    geo::Mesh mesh;

    // Build a mesh with one face
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    // Same name on both stores must not collide
    mesh.addBoolAttribute(attr::AttrOwner::FACE, "selected");
    mesh.createGroup(attr::AttrOwner::FACE, "selected");

    auto attribute = mesh.getAttribByName(attr::AttrOwner::FACE, "selected");
    auto group = mesh.getGroupByName(attr::AttrOwner::FACE, "selected");

    REQUIRE(attribute != nullptr);
    REQUIRE(group != nullptr);
    REQUIRE(attribute != group);
}

TEST_CASE("Group survives defragment")
{
    geo::Mesh mesh;

    // Build a mesh with three faces, put faces 0 and 2 in a group
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    mesh.createGroup(attr::AttrOwner::FACE, "g");
    mesh.addToGroup(attr::AttrOwner::FACE, "g", {0, 2});

    // Drop the middle face and compact
    mesh.deleteFaces({1});
    mesh.defragment();

    // After defrag old face 0 stays at offset 0 and old face 2 shifts to offset 1.
    // Both should still report as members of the group.
    attr::AttributeHandleBool handle(mesh.getGroupByName(attr::AttrOwner::FACE, "g"));
    REQUIRE(handle.getSize() == 2);
    REQUIRE(handle.getValue(0) == true);
    REQUIRE(handle.getValue(1) == true);
}

TEST_CASE("Add faces")
{
    geo::Mesh mesh;

    // Build the source points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 1, 0));

    // Two triangles in a single batch call
    std::vector<Offset> pointOffsetsFlat{
        pointOffset0,
        pointOffset1,
        pointOffset2,
        pointOffset1,
        pointOffset2,
        pointOffset3
    };
    std::vector<Offset> vertexCounts{3, 3};
    auto faceOffsets = mesh.addFaces(pointOffsetsFlat, vertexCounts);

    // Returned offsets should be the new face indices
    REQUIRE(faceOffsets.size() == 2);
    REQUIRE(faceOffsets[0] == 0);
    REQUIRE(faceOffsets[1] == 1);
    REQUIRE(mesh.getNumFaces() == 2);

    // Each face should report the right point references
    auto face0Points = mesh.getFacePoints(faceOffsets[0]);
    REQUIRE(face0Points.size() == 3);
    REQUIRE(face0Points[0] == static_cast<intT>(pointOffset0));
    REQUIRE(face0Points[1] == static_cast<intT>(pointOffset1));
    REQUIRE(face0Points[2] == static_cast<intT>(pointOffset2));

    auto face1Points = mesh.getFacePoints(faceOffsets[1]);
    REQUIRE(face1Points.size() == 3);
    REQUIRE(face1Points[0] == static_cast<intT>(pointOffset1));
    REQUIRE(face1Points[1] == static_cast<intT>(pointOffset2));
    REQUIRE(face1Points[2] == static_cast<intT>(pointOffset3));
}

TEST_CASE("Add faces mixed sizes")
{
    geo::Mesh mesh;

    // Build the source points
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 1, 0));
    auto pointOffset4 = mesh.addPoint(Vector3(2, 0, 0));

    // One quad, one tri
    std::vector<Offset> pointOffsetsFlat{
        pointOffset0,
        pointOffset1,
        pointOffset2,
        pointOffset3,
        pointOffset0,
        pointOffset1,
        pointOffset4
    };
    std::vector<Offset> vertexCounts{4, 3};
    auto faceOffsets = mesh.addFaces(pointOffsetsFlat, vertexCounts);

    // The quad keeps its four points in order
    auto quadPoints = mesh.getFacePoints(faceOffsets[0]);
    REQUIRE(quadPoints.size() == 4);
    REQUIRE(quadPoints[3] == static_cast<intT>(pointOffset3));

    // The tri starts at its own primStart, not appended to the quad
    auto triPoints = mesh.getFacePoints(faceOffsets[1]);
    REQUIRE(triPoints.size() == 3);
    REQUIRE(triPoints[0] == static_cast<intT>(pointOffset0));
    REQUIRE(triPoints[2] == static_cast<intT>(pointOffset4));
}

TEST_CASE("Add faces resizes group")
{
    geo::Mesh mesh;

    // Build a mesh with two faces, both in a group
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    mesh.createGroup(attr::AttrOwner::FACE, "g");
    mesh.addToGroup(attr::AttrOwner::FACE, "g", {0, 1});

    // Add one more face via the batch path
    std::vector<Offset> pointOffsetsFlat{pointOffset0, pointOffset1, pointOffset2};
    std::vector<Offset> vertexCounts{3};
    mesh.addFaces(pointOffsetsFlat, vertexCounts);

    // The group must have grown to cover the new face, and that face
    // should default to non-member.
    attr::AttributeHandleBool handle(mesh.getGroupByName(attr::AttrOwner::FACE, "g"));
    REQUIRE(handle.getSize() == 3);
    REQUIRE(handle.getValue(0) == true);
    REQUIRE(handle.getValue(1) == true);
    REQUIRE(handle.getValue(2) == false);
}

TEST_CASE("Face normal of axis aligned quad")
{
    geo::Mesh mesh;

    // CCW from +Y so the right hand rule normal is +Y
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(0, 0, 1));
    auto pointOffset2 = mesh.addPoint(Vector3(1, 0, 1));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 0, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2, pointOffset3});

    auto faceNormals = mesh.getFaceNormal();
    REQUIRE(faceNormals[0] == Vector3(0, 1, 0));
}

TEST_CASE("Face normal of tilted triangle uses Newell's method")
{
    geo::Mesh mesh;

    // Triangle in the plane y = -z (normal direction (0, -1, 1)/sqrt(2))
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(1, 0, 0));
    auto pointOffset2 = mesh.addPoint(Vector3(0, 1, 1));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});

    auto faceNormals = mesh.getFaceNormal();
    const Vector3 normal = faceNormals[0];
    const double oneOverRoot2 = 1.0 / std::sqrt(2.0);
    const double eps = 1e-6;
    REQUIRE(std::abs(normal.x()) < eps);
    REQUIRE(std::abs(normal.y() + oneOverRoot2) < eps);
    REQUIRE(std::abs(normal.z() - oneOverRoot2) < eps);
}

TEST_CASE("Face normal prefers Normal attribute over Newell")
{
    geo::Mesh mesh;

    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(0, 0, 1));
    auto pointOffset2 = mesh.addPoint(Vector3(1, 0, 1));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 0, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2, pointOffset3});

    // Newell would give (0, 1, 0) for this face. Override with a different
    // direction so we can tell which path was used.
    auto normalAttr = mesh.addVector3Attribute(attr::AttrOwner::FACE, "Normal");
    normalAttr.setValue(0, Vector3(1, 0, 0));

    auto faceNormals = mesh.getFaceNormal();
    REQUIRE(faceNormals[0] == Vector3(1, 0, 0));
}

TEST_CASE("Face normal precompute path matches on the fly")
{
    geo::Mesh mesh;

    // Two faces with different orientations
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(0, 0, 1));
    auto pointOffset2 = mesh.addPoint(Vector3(1, 0, 1));
    auto pointOffset3 = mesh.addPoint(Vector3(0, 1, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2});
    mesh.addFace({pointOffset0, pointOffset3, pointOffset1});

    auto onTheFly = mesh.getFaceNormal(false);
    auto precomputed = mesh.getFaceNormal(true);

    REQUIRE(onTheFly[0] == precomputed[0]);
    REQUIRE(onTheFly[1] == precomputed[1]);
}

TEST_CASE("Vertex normal falls back to owning face")
{
    geo::Mesh mesh;

    // Axis aligned quad, face normal is +Y
    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(0, 0, 1));
    auto pointOffset2 = mesh.addPoint(Vector3(1, 0, 1));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 0, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2, pointOffset3});

    auto vertexNormals = mesh.getVertexNormal();
    REQUIRE(vertexNormals[0] == Vector3(0, 1, 0));
    REQUIRE(vertexNormals[1] == Vector3(0, 1, 0));
    REQUIRE(vertexNormals[2] == Vector3(0, 1, 0));
    REQUIRE(vertexNormals[3] == Vector3(0, 1, 0));
}

TEST_CASE("Vertex normal prefers Normal attribute over face fallback")
{
    geo::Mesh mesh;

    auto pointOffset0 = mesh.addPoint(Vector3(0, 0, 0));
    auto pointOffset1 = mesh.addPoint(Vector3(0, 0, 1));
    auto pointOffset2 = mesh.addPoint(Vector3(1, 0, 1));
    auto pointOffset3 = mesh.addPoint(Vector3(1, 0, 0));
    mesh.addFace({pointOffset0, pointOffset1, pointOffset2, pointOffset3});

    // Override one vertex's normal so we can see the attribute path wins
    auto normalAttr = mesh.addVector3Attribute(attr::AttrOwner::VERTEX, "Normal");
    normalAttr.setValue(0, Vector3(1, 0, 0));
    normalAttr.setValue(1, Vector3(0, 1, 0));
    normalAttr.setValue(2, Vector3(0, 0, 1));
    normalAttr.setValue(3, Vector3(-1, 0, 0));

    auto vertexNormals = mesh.getVertexNormal();
    REQUIRE(vertexNormals[0] == Vector3(1, 0, 0));
    REQUIRE(vertexNormals[1] == Vector3(0, 1, 0));
    REQUIRE(vertexNormals[2] == Vector3(0, 0, 1));
    REQUIRE(vertexNormals[3] == Vector3(-1, 0, 0));
}
