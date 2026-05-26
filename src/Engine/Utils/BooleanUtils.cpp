#include "Engine/Utils/BooleanUtils.h"
#include "Engine/Utils/MeshUtils.h"
#include "Engine/Operator/Mesh.h"

#include <manifold/manifold.h>

#include <cstdint>
#include <iostream>
#include <vector>

namespace enzo::utils {

namespace {

// Pack a triangulated enzo mesh into a Manifold MeshGL64. Each output triangle's
// faceID is faceIdOffset plus its source face offset, so all triangles of one
// source polygon share a faceID through the boolean and any downstream detriangulation.
manifold::MeshGL64 packMeshGL(const geo::Mesh& triangulated,
                              const std::vector<ga::Offset>& faceToOriginal,
                              uint64_t faceIdOffset)
{
    manifold::MeshGL64 meshGL;
    meshGL.numProp = 3;

    const ga::Offset numPoints = triangulated.getNumPoints();
    meshGL.vertProperties.reserve(static_cast<size_t>(numPoints) * 3);
    for (ga::Offset pointOffset = 0; pointOffset < numPoints; ++pointOffset)
    {
        const bt::Vector3 pos = triangulated.getPointPos(pointOffset);
        meshGL.vertProperties.push_back(pos.x());
        meshGL.vertProperties.push_back(pos.y());
        meshGL.vertProperties.push_back(pos.z());
    }

    const ga::Offset numFaces = triangulated.getNumFaces();
    meshGL.triVerts.reserve(static_cast<size_t>(numFaces) * 3);
    meshGL.faceID.reserve(static_cast<size_t>(numFaces));
    for (ga::Offset faceOffset = 0; faceOffset < numFaces; ++faceOffset)
    {
        const std::span<const bt::intT> facePoints = triangulated.getFacePoints(faceOffset);
        meshGL.triVerts.push_back(static_cast<uint64_t>(facePoints[0]));
        meshGL.triVerts.push_back(static_cast<uint64_t>(facePoints[1]));
        meshGL.triVerts.push_back(static_cast<uint64_t>(facePoints[2]));
        meshGL.faceID.push_back(faceIdOffset + static_cast<uint64_t>(faceToOriginal[faceOffset]));
    }
    return meshGL;
}

manifold::OpType toManifoldOp(BooleanOp op)
{
    switch (op)
    {
        case BooleanOp::INTERSECT: return manifold::OpType::Intersect;
        case BooleanOp::SUBTRACT:  return manifold::OpType::Subtract;
        case BooleanOp::UNION:
        default:                   return manifold::OpType::Add;
    }
}

} // namespace

std::shared_ptr<geo::Mesh> booleanMesh(const geo::Mesh& meshA,
                                       const geo::Mesh& meshB,
                                       BooleanOp op)
{
    // Triangulate each input. faceToOriginal maps each triangle back to the source
    // polygon so the faceID attached below stays coherent across a fan.
    const TriangulatedMesh triA = triangulateMesh(meshA);
    const TriangulatedMesh triB = triangulateMesh(meshB);

    // Offset B's IDs past A's range so the two id spaces never collide. Triangles
    // from A then carry faceID below aIdEnd and triangles from B carry faceID at
    // or above aIdEnd.
    const uint64_t aIdEnd = static_cast<uint64_t>(meshA.getNumFaces());
    manifold::MeshGL64 meshGLA = packMeshGL(*triA.mesh, triA.faceToOriginal, 0);
    manifold::MeshGL64 meshGLB = packMeshGL(*triB.mesh, triB.faceToOriginal, aIdEnd);

    manifold::Manifold manA(meshGLA);
    manifold::Manifold manB(meshGLB);
    if (manA.Status() != manifold::Manifold::Error::NoError)
        std::cerr << "[boolean] input A status code: " << static_cast<int>(manA.Status()) << "\n";
    if (manB.Status() != manifold::Manifold::Error::NoError)
        std::cerr << "[boolean] input B status code: " << static_cast<int>(manB.Status()) << "\n";

    manifold::Manifold resultManifold = manA.Boolean(manB, toManifoldOp(op));
    if (resultManifold.Status() != manifold::Manifold::Error::NoError)
        std::cerr << "[boolean] result status code: " << static_cast<int>(resultManifold.Status()) << "\n";

    manifold::MeshGL64 resultGL = resultManifold.GetMeshGL64();
    std::cerr << "[boolean] op=" << static_cast<int>(op)
              << " inA tris=" << meshGLA.triVerts.size() / 3
              << " inB tris=" << meshGLB.triVerts.size() / 3
              << " out tris=" << resultGL.triVerts.size() / 3
              << " out verts=" << (resultGL.numProp == 0 ? 0 : resultGL.vertProperties.size() / resultGL.numProp)
              << " merges=" << resultGL.mergeFromVert.size() << "\n";

    // Build the output enzo mesh. Every result triangle becomes its own face.
    auto outMesh = std::make_shared<geo::Mesh>(meshA.getPath());

    const size_t numProp = resultGL.numProp;
    const size_t numVerts = numProp == 0 ? 0 : resultGL.vertProperties.size() / numProp;
    std::vector<bt::Vector3> positions;
    positions.reserve(numVerts);
    for (size_t vertIndex = 0; vertIndex < numVerts; ++vertIndex)
    {
        const size_t baseIndex = vertIndex * numProp;
        positions.emplace_back(resultGL.vertProperties[baseIndex + 0],
                               resultGL.vertProperties[baseIndex + 1],
                               resultGL.vertProperties[baseIndex + 2]);
    }
    outMesh->addPoints(positions);

    // Manifold returns separate vertex indices for points that share a position
    // along boolean cuts. Remap mergeFromVert[i] to mergeToVert[i] so adjacent
    // triangles share vertices and the mesh stays closed.
    std::vector<uint64_t> vertexRemap(numVerts);
    for (size_t vertIndex = 0; vertIndex < numVerts; ++vertIndex) vertexRemap[vertIndex] = vertIndex;
    for (size_t mergeIndex = 0; mergeIndex < resultGL.mergeFromVert.size(); ++mergeIndex)
    {
        const uint64_t fromVert = resultGL.mergeFromVert[mergeIndex];
        const uint64_t toVert = resultGL.mergeToVert[mergeIndex];
        if (fromVert < vertexRemap.size()) vertexRemap[fromVert] = toVert;
    }

    const size_t numTris = resultGL.triVerts.size() / 3;
    std::vector<ga::Offset> triPointsFlat;
    std::vector<ga::Offset> triVertexCounts;
    triPointsFlat.reserve(numTris * 3);
    triVertexCounts.reserve(numTris);
    for (size_t triIndex = 0; triIndex < numTris; ++triIndex)
    {
        triPointsFlat.push_back(static_cast<ga::Offset>(vertexRemap[resultGL.triVerts[triIndex * 3 + 0]]));
        triPointsFlat.push_back(static_cast<ga::Offset>(vertexRemap[resultGL.triVerts[triIndex * 3 + 1]]));
        triPointsFlat.push_back(static_cast<ga::Offset>(vertexRemap[resultGL.triVerts[triIndex * 3 + 2]]));
        triVertexCounts.push_back(3);
    }
    outMesh->addFaces(triPointsFlat, triVertexCounts);

    return outMesh;
}

} // namespace enzo::utils
