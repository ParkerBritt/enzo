#include "Engine/Utils/BooleanUtils.h"
#include "Engine/Utils/MeshUtils.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"

#include <manifold/manifold.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enzo::utils {

namespace {

// Backend neutral intermediate. A soup of result triangles whose vertices live
// in a single shared position list and whose source faces are tagged. Both
// Manifold and a future CGAL clip backend can produce this shape.
struct SourceFace
{
    enum class Side { A, B, NEW };
    Side side = Side::NEW;
    Offset faceOffset = 0;
};

struct BooleanFragments
{
    std::vector<Vector3> positions;
    std::vector<std::array<int, 3>> triangles;
    std::vector<SourceFace> triSourceFace;
};

// Origin tag for a single result vertex.
struct VertexOrigin
{
    enum class Kind { ORIGINAL, ON_EDGE, NEW };
    enum class Side { A, B, NONE };
    Kind kind = Kind::NEW;
    Side side = Side::NONE;
    Offset endpoint0 = 0;
    Offset endpoint1 = 0;
    double t = 0.0;
};

// A reconstructed polygon. An ordered loop of fragment vertex indices plus a
// pointer back to the source face that produced its triangles.
struct DetriangulatedFace
{
    SourceFace source;
    std::vector<int> loop;
};


// Pack one triangulated enzo mesh into a Manifold MeshGL64 and tag each tri's
// faceID so the result still knows which source polygon every triangle
// belonged to. The id offset lets caller A occupy [0, |A|) and caller B
// occupy [|A|, |A|+|B|).
manifold::MeshGL64 packMeshGL(const geo::Mesh& triangulated,
                              const std::vector<Offset>& faceToOriginal,
                              uint64_t faceIdOffset)
{
    manifold::MeshGL64 meshGL;
    meshGL.numProp = 3;

    // Copy every triangulated vertex position.
    const Offset numPoints = triangulated.getNumPoints();
    meshGL.vertProperties.reserve(static_cast<size_t>(numPoints) * 3);
    for (Offset pointOffset = 0; pointOffset < numPoints; ++pointOffset)
    {
        const Vector3 pos = triangulated.getPointPos(pointOffset);
        meshGL.vertProperties.push_back(pos.x());
        meshGL.vertProperties.push_back(pos.y());
        meshGL.vertProperties.push_back(pos.z());
    }

    // Emit every triangulated face as a tri and tag it with its source face id.
    const Offset numFaces = triangulated.getNumFaces();
    meshGL.triVerts.reserve(static_cast<size_t>(numFaces) * 3);
    meshGL.faceID.reserve(static_cast<size_t>(numFaces));
    for (Offset faceOffset = 0; faceOffset < numFaces; ++faceOffset)
    {
        const std::span<const intT> facePoints = triangulated.getFacePoints(faceOffset);
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


// Human readable name for a Manifold status code, used when reporting a
// boolean failure back to the node.
const char* manifoldStatusString(manifold::Manifold::Error status)
{
    using Error = manifold::Manifold::Error;
    switch (status)
    {
        case Error::NoError:                      return "no error";
        case Error::NonFiniteVertex:              return "non finite vertex";
        case Error::NotManifold:                  return "not manifold";
        case Error::VertexOutOfBounds:            return "vertex index out of bounds";
        case Error::PropertiesWrongLength:        return "properties wrong length";
        case Error::MissingPositionProperties:    return "missing position properties";
        case Error::MergeVectorsDifferentLengths: return "merge vectors different lengths";
        case Error::MergeIndexOutOfBounds:        return "merge index out of bounds";
        case Error::TransformWrongLength:         return "transform wrong length";
        case Error::RunIndexWrongLength:          return "run index wrong length";
        case Error::FaceIDWrongLength:            return "face id wrong length";
        case Error::InvalidConstruction:          return "invalid construction";
        case Error::ResultTooLarge:               return "result too large";
    }
    return "unknown error";
}

// Append one line to the optional error sink. A non-null, non-empty sink at
// the end of the boolean means the node should report a cook failure.
void appendError(std::string* error, const std::string& message)
{
    if (!error) return;
    if (!error->empty()) *error += "\n";
    *error += message;
}

// Append a labeled Manifold status line to the optional error sink.
void reportStatus(std::string* error, const char* label, manifold::Manifold::Error status)
{
    appendError(error, std::string(label) + ": " + manifoldStatusString(status));
}


// Run Manifold's boolean and convert the MeshGL64 result into our backend
// neutral intermediate. The merge table is folded in so duplicate vertices
// along boolean cuts get a single canonical index. Any Manifold status failure
// is written to @p error so the calling node can surface it.
BooleanFragments runManifold(const geo::Mesh& meshA, const geo::Mesh& meshB, BooleanOp op,
                             std::string* error)
{
    // Triangulate each input. faceToOriginal maps each fan triangle back to its source polygon.
    const TriangulatedMesh triA = triangulateMesh(meshA);
    const TriangulatedMesh triB = triangulateMesh(meshB);

    // Pack into Manifold with disjoint id ranges so A and B never collide.
    const uint64_t aIdEnd = static_cast<uint64_t>(meshA.getNumFaces());
    manifold::MeshGL64 meshGLA = packMeshGL(*triA.mesh, triA.faceToOriginal, 0);
    manifold::MeshGL64 meshGLB = packMeshGL(*triB.mesh, triB.faceToOriginal, aIdEnd);

    manifold::Manifold manA(meshGLA);
    manifold::Manifold manB(meshGLB);
    if (manA.Status() != manifold::Manifold::Error::NoError)
        reportStatus(error, "input A", manA.Status());
    if (manB.Status() != manifold::Manifold::Error::NoError)
        reportStatus(error, "input B", manB.Status());

    manifold::Manifold resultManifold = manA.Boolean(manB, toManifoldOp(op));
    if (resultManifold.Status() != manifold::Manifold::Error::NoError)
        reportStatus(error, "result", resultManifold.Status());

    manifold::MeshGL64 resultGL = resultManifold.GetMeshGL64();

    BooleanFragments fragments;
    const size_t numProp = resultGL.numProp;
    const size_t numVerts = numProp == 0 ? 0 : resultGL.vertProperties.size() / numProp;

    // Copy raw vertex positions into the fragments.
    fragments.positions.reserve(numVerts);
    for (size_t vertIndex = 0; vertIndex < numVerts; ++vertIndex)
    {
        const size_t baseIndex = vertIndex * numProp;
        fragments.positions.emplace_back(resultGL.vertProperties[baseIndex + 0],
                                          resultGL.vertProperties[baseIndex + 1],
                                          resultGL.vertProperties[baseIndex + 2]);
    }

    // Apply Manifold's merge so duplicates collapse to a single canonical index.
    std::vector<int> vertexRemap(numVerts);
    for (size_t vertIndex = 0; vertIndex < numVerts; ++vertIndex)
        vertexRemap[vertIndex] = static_cast<int>(vertIndex);
    for (size_t mergeIndex = 0; mergeIndex < resultGL.mergeFromVert.size(); ++mergeIndex)
    {
        const uint64_t fromVert = resultGL.mergeFromVert[mergeIndex];
        const uint64_t toVert = resultGL.mergeToVert[mergeIndex];
        if (fromVert < vertexRemap.size()) vertexRemap[fromVert] = static_cast<int>(toVert);
    }

    // Emit triangles with remapped indices, tagging each by its source face.
    const size_t numTris = resultGL.triVerts.size() / 3;
    fragments.triangles.reserve(numTris);
    fragments.triSourceFace.reserve(numTris);
    for (size_t triIndex = 0; triIndex < numTris; ++triIndex)
    {
        std::array<int, 3> tri = {
            vertexRemap[resultGL.triVerts[triIndex * 3 + 0]],
            vertexRemap[resultGL.triVerts[triIndex * 3 + 1]],
            vertexRemap[resultGL.triVerts[triIndex * 3 + 2]],
        };
        fragments.triangles.push_back(tri);

        // Decode the per-tri faceID back into A side or B side.
        SourceFace source;
        if (triIndex < resultGL.faceID.size())
        {
            const uint64_t faceId = resultGL.faceID[triIndex];
            if (faceId < aIdEnd)
            {
                source.side = SourceFace::Side::A;
                source.faceOffset = static_cast<Offset>(faceId);
            }
            else
            {
                source.side = SourceFace::Side::B;
                source.faceOffset = static_cast<Offset>(faceId - aIdEnd);
            }
        }
        fragments.triSourceFace.push_back(source);
    }

    return fragments;
}


// Spatial key for hashing positions to (side, point offset) lookups. Coords
// are quantized so Manifold-induced micro-drift between identical points still
// hashes to the same bucket.
struct PositionKey
{
    int64_t x = 0;
    int64_t y = 0;
    int64_t z = 0;
    bool operator==(const PositionKey& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};
struct PositionKeyHash
{
    size_t operator()(const PositionKey& key) const noexcept
    {
        size_t combined = std::hash<int64_t>{}(key.x);
        combined ^= std::hash<int64_t>{}(key.y) + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2);
        combined ^= std::hash<int64_t>{}(key.z) + 0x9e3779b97f4a7c15ULL + (combined << 6) + (combined >> 2);
        return combined;
    }
};
constexpr double POSITION_HASH_SCALE = 1.0e7;
PositionKey quantizePosition(const Vector3& position)
{
    PositionKey key;
    key.x = static_cast<int64_t>(std::llround(position.x() * POSITION_HASH_SCALE));
    key.y = static_cast<int64_t>(std::llround(position.y() * POSITION_HASH_SCALE));
    key.z = static_cast<int64_t>(std::llround(position.z() * POSITION_HASH_SCALE));
    return key;
}


// Find every unique undirected edge in a mesh by walking its faces.
struct EdgePair
{
    Offset endpoint0 = 0;
    Offset endpoint1 = 0;
};
std::vector<EdgePair> collectEdges(const geo::Mesh& mesh)
{
    std::unordered_set<uint64_t> seen;
    std::vector<EdgePair> edges;
    const Offset numFaces = mesh.getNumFaces();
    for (Offset faceOffset = 0; faceOffset < numFaces; ++faceOffset)
    {
        if (!mesh.isValidFace(faceOffset)) continue;
        const std::span<const intT> facePoints = mesh.getFacePoints(faceOffset);
        const size_t cornerCount = facePoints.size();
        for (size_t cornerIndex = 0; cornerIndex < cornerCount; ++cornerIndex)
        {
            const Offset endpoint0 = facePoints[cornerIndex];
            const Offset endpoint1 = facePoints[(cornerIndex + 1) % cornerCount];
            const Offset lo = std::min(endpoint0, endpoint1);
            const Offset hi = std::max(endpoint0, endpoint1);
            const uint64_t key = (static_cast<uint64_t>(lo) << 32) | static_cast<uint64_t>(hi);
            if (seen.insert(key).second)
            {
                edges.push_back({lo, hi});
            }
        }
    }
    return edges;
}


// Decide whether a result vertex sits inside the given segment. Returns true
// with `outT` set in [0, 1] when the perpendicular distance is below eps.
bool pointOnSegment(const Vector3& point,
                    const Vector3& start,
                    const Vector3& end,
                    double& outT)
{
    const Vector3 direction = end - start;
    const double lengthSquared = direction.squaredNorm();
    if (lengthSquared <= 0) return false;

    const Vector3 offset = point - start;
    const double rawT = offset.dot(direction) / lengthSquared;
    if (rawT < -1e-7 || rawT > 1.0 + 1e-7) return false;

    const Vector3 closest = start + direction * rawT;
    const double perpendicularSq = (point - closest).squaredNorm();
    if (perpendicularSq > 1e-12) return false;

    outT = std::clamp(rawT, 0.0, 1.0);
    return true;
}


// For every fragment vertex decide whether it is an original A or B point, a
// point sitting on an A or B edge (with parameter t), or genuinely new.
std::vector<VertexOrigin> resolveOrigins(const BooleanFragments& fragments,
                                         const geo::Mesh& meshA,
                                         const geo::Mesh& meshB)
{
    // Build a position hash from A then B input points.
    std::unordered_map<PositionKey, std::pair<VertexOrigin::Side, Offset>, PositionKeyHash> positionToPoint;
    for (Offset pointOffset = 0; pointOffset < meshA.getNumPoints(); ++pointOffset)
    {
        if (!meshA.isValidPoint(pointOffset)) continue;
        positionToPoint.emplace(quantizePosition(meshA.getPointPos(pointOffset)),
                                std::make_pair(VertexOrigin::Side::A, pointOffset));
    }
    for (Offset pointOffset = 0; pointOffset < meshB.getNumPoints(); ++pointOffset)
    {
        if (!meshB.isValidPoint(pointOffset)) continue;
        // Only insert if A didn't already claim the position. This isn't a
        // hard rule, but it gives A points priority on shared coordinates.
        positionToPoint.emplace(quantizePosition(meshB.getPointPos(pointOffset)),
                                std::make_pair(VertexOrigin::Side::B, pointOffset));
    }

    // Collect edges once so unmatched fragment vertices can scan against them.
    const std::vector<EdgePair> edgesA = collectEdges(meshA);
    const std::vector<EdgePair> edgesB = collectEdges(meshB);

    std::vector<VertexOrigin> origins(fragments.positions.size());
    for (size_t vertIndex = 0; vertIndex < fragments.positions.size(); ++vertIndex)
    {
        const Vector3& position = fragments.positions[vertIndex];

        // First check if the vertex hashes to an original input point.
        const auto match = positionToPoint.find(quantizePosition(position));
        if (match != positionToPoint.end())
        {
            VertexOrigin origin;
            origin.kind = VertexOrigin::Kind::ORIGINAL;
            origin.side = match->second.first;
            origin.endpoint0 = match->second.second;
            origins[vertIndex] = origin;
            continue;
        }

        // Otherwise look for an A or B edge that contains this vertex.
        auto tryEdges = [&](const std::vector<EdgePair>& edges,
                            const geo::Mesh& sourceMesh,
                            VertexOrigin::Side side,
                            VertexOrigin& outOrigin) -> bool
        {
            for (const EdgePair& edge : edges)
            {
                const Vector3 start = sourceMesh.getPointPos(edge.endpoint0);
                const Vector3 end   = sourceMesh.getPointPos(edge.endpoint1);
                double parametricT = 0.0;
                if (pointOnSegment(position, start, end, parametricT))
                {
                    outOrigin.kind = VertexOrigin::Kind::ON_EDGE;
                    outOrigin.side = side;
                    outOrigin.endpoint0 = edge.endpoint0;
                    outOrigin.endpoint1 = edge.endpoint1;
                    outOrigin.t = parametricT;
                    return true;
                }
            }
            return false;
        };

        VertexOrigin origin;
        if (tryEdges(edgesA, meshA, VertexOrigin::Side::A, origin) ||
            tryEdges(edgesB, meshB, VertexOrigin::Side::B, origin))
        {
            origins[vertIndex] = origin;
            continue;
        }

        // Fragment vertex belongs to no original point or edge.
        origins[vertIndex] = VertexOrigin{};
    }
    return origins;
}


// Key that packs a SourceFace into one integer so we can bucket by it.
struct SourceFaceKey
{
    SourceFace::Side side;
    Offset faceOffset;
    bool operator==(const SourceFaceKey& other) const
    {
        return side == other.side && faceOffset == other.faceOffset;
    }
};
struct SourceFaceKeyHash
{
    size_t operator()(const SourceFaceKey& key) const noexcept
    {
        return std::hash<Offset>{}(key.faceOffset) ^ (static_cast<size_t>(key.side) * 0x9e3779b9);
    }
};


// Group triangles by source face, then walk each group's boundary edges into
// closed loops. Interior fan edges cancel since each appears in both winding
// directions so only original polygon edges and cut edges remain. A boundary
// that fails to close into a polygon is reported through @p error rather than
// dropped, since a dropped face would leave a hole in the result.
std::vector<DetriangulatedFace> detriangulate(const BooleanFragments& fragments,
                                              std::string* error)
{
    // Bucket triangle indices by source face.
    std::unordered_map<SourceFaceKey, std::vector<size_t>, SourceFaceKeyHash> trisBySource;
    for (size_t triIndex = 0; triIndex < fragments.triangles.size(); ++triIndex)
    {
        const SourceFace& source = fragments.triSourceFace[triIndex];
        trisBySource[{source.side, source.faceOffset}].push_back(triIndex);
    }

    std::vector<DetriangulatedFace> faces;
    for (auto& [key, triIndices] : trisBySource)
    {
        // Build a directed edge multiset across every triangle in this bucket.
        // Adding (a,b) cancels one prior (b,a). Whatever remains is boundary.
        std::unordered_map<int, std::unordered_multiset<int>> outgoingEdges;
        auto addDirectedEdge = [&](int fromVert, int toVert)
        {
            auto reverseIt = outgoingEdges.find(toVert);
            if (reverseIt != outgoingEdges.end())
            {
                auto cancelIt = reverseIt->second.find(fromVert);
                if (cancelIt != reverseIt->second.end())
                {
                    reverseIt->second.erase(cancelIt);
                    return;
                }
            }
            outgoingEdges[fromVert].insert(toVert);
        };
        for (size_t triIndex : triIndices)
        {
            const std::array<int, 3>& tri = fragments.triangles[triIndex];
            addDirectedEdge(tri[0], tri[1]);
            addDirectedEdge(tri[1], tri[2]);
            addDirectedEdge(tri[2], tri[0]);
        }

        // Walk remaining edges into one or more closed loops.
        while (true)
        {
            // Find any vertex that still has an outgoing boundary edge.
            int startVert = -1;
            for (auto& [fromVert, neighbors] : outgoingEdges)
            {
                if (!neighbors.empty()) { startVert = fromVert; break; }
            }
            if (startVert < 0) break;

            std::vector<int> loop;
            int currentVert = startVert;
            const int safetyLimit = static_cast<int>(triIndices.size()) * 3 + 4;
            int safetyCounter = 0;
            bool closed = false;
            while (safetyCounter++ < safetyLimit)
            {
                auto& neighbors = outgoingEdges[currentVert];
                if (neighbors.empty()) break;
                const int nextVert = *neighbors.begin();
                neighbors.erase(neighbors.begin());
                loop.push_back(currentVert);
                currentVert = nextVert;
                if (currentVert == startVert) { closed = true; break; }
            }

            // A valid polygon is a closed loop with at least three distinct corners.
            // Anything else means the cut left a boundary we can't rebuild, so
            // report it instead of emitting a hole.
            if (closed && loop.size() >= 3)
            {
                DetriangulatedFace face;
                face.source.side = key.side;
                face.source.faceOffset = key.faceOffset;
                face.loop = std::move(loop);
                faces.push_back(std::move(face));
            }
            else
            {
                const char* sideName = key.side == SourceFace::Side::A ? "A" : "B";
                appendError(error, "detriangulation could not close a boundary on source face "
                                       + std::to_string(key.faceOffset) + " of input " + sideName);
            }
        }
    }

    return faces;
}


// Generic copy of one row from a source attribute store into a destination
// attribute store, matching attributes by name and dispatching on type.
void copyAttributeRow(const attr::attribVector& sourceStore,
                      attr::attribVector& destStore,
                      Offset sourceOffset,
                      Offset destOffset)
{
    for (const auto& sourceAttribute : sourceStore)
    {
        if (!sourceAttribute) continue;
        if (sourceAttribute->isIntrinsic()) continue;

        // Find a matching destination attribute by name. Skip if it doesn't exist.
        std::shared_ptr<attr::Attribute> destAttribute;
        for (const auto& candidate : destStore)
        {
            if (candidate && candidate->getName() == sourceAttribute->getName())
            {
                destAttribute = candidate;
                break;
            }
        }
        if (!destAttribute) continue;
        if (destAttribute->getType() != sourceAttribute->getType()) continue;

        switch (sourceAttribute->getType())
        {
            case attr::AttributeType::intT:
            {
                attr::AttributeHandleRO<intT> sourceHandle(sourceAttribute);
                attr::AttributeHandle<intT> destHandle(destAttribute);
                destHandle.setValue(destOffset, sourceHandle.getValue(sourceOffset));
                break;
            }
            case attr::AttributeType::floatT:
            {
                attr::AttributeHandleRO<floatT> sourceHandle(sourceAttribute);
                attr::AttributeHandle<floatT> destHandle(destAttribute);
                destHandle.setValue(destOffset, sourceHandle.getValue(sourceOffset));
                break;
            }
            case attr::AttributeType::vectorT:
            {
                attr::AttributeHandleRO<Vector3> sourceHandle(sourceAttribute);
                attr::AttributeHandle<Vector3> destHandle(destAttribute);
                destHandle.setValue(destOffset, sourceHandle.getValue(sourceOffset));
                break;
            }
            case attr::AttributeType::boolT:
            {
                attr::AttributeHandleRO<boolT> sourceHandle(sourceAttribute);
                attr::AttributeHandle<boolT> destHandle(destAttribute);
                destHandle.setValue(destOffset, sourceHandle.getValue(sourceOffset));
                break;
            }
            case attr::AttributeType::matrixT:
            {
                attr::AttributeHandleRO<Matrix4> sourceHandle(sourceAttribute);
                attr::AttributeHandle<Matrix4> destHandle(destAttribute);
                destHandle.setValue(destOffset, sourceHandle.getValue(sourceOffset));
                break;
            }
            default: break;
        }
    }
}


// Same as copyAttributeRow but writes a linear blend of two source rows,
// useful for cut points sitting on an edge of an input mesh.
void interpolateAttributeRow(const attr::attribVector& sourceStore,
                              attr::attribVector& destStore,
                              Offset endpointOffset0,
                              Offset endpointOffset1,
                              double parametricT,
                              Offset destOffset)
{
    const double weight1 = parametricT;
    const double weight0 = 1.0 - parametricT;

    for (const auto& sourceAttribute : sourceStore)
    {
        if (!sourceAttribute) continue;
        if (sourceAttribute->isIntrinsic()) continue;

        std::shared_ptr<attr::Attribute> destAttribute;
        for (const auto& candidate : destStore)
        {
            if (candidate && candidate->getName() == sourceAttribute->getName())
            {
                destAttribute = candidate;
                break;
            }
        }
        if (!destAttribute) continue;
        if (destAttribute->getType() != sourceAttribute->getType()) continue;

        switch (sourceAttribute->getType())
        {
            case attr::AttributeType::intT:
            {
                attr::AttributeHandleRO<intT> sourceHandle(sourceAttribute);
                attr::AttributeHandle<intT> destHandle(destAttribute);
                const double blended = sourceHandle.getValue(endpointOffset0) * weight0
                                     + sourceHandle.getValue(endpointOffset1) * weight1;
                destHandle.setValue(destOffset, static_cast<intT>(std::llround(blended)));
                break;
            }
            case attr::AttributeType::floatT:
            {
                attr::AttributeHandleRO<floatT> sourceHandle(sourceAttribute);
                attr::AttributeHandle<floatT> destHandle(destAttribute);
                const double blended = sourceHandle.getValue(endpointOffset0) * weight0
                                     + sourceHandle.getValue(endpointOffset1) * weight1;
                destHandle.setValue(destOffset, blended);
                break;
            }
            case attr::AttributeType::vectorT:
            {
                attr::AttributeHandleRO<Vector3> sourceHandle(sourceAttribute);
                attr::AttributeHandle<Vector3> destHandle(destAttribute);
                const Vector3 blended = sourceHandle.getValue(endpointOffset0) * weight0
                                           + sourceHandle.getValue(endpointOffset1) * weight1;
                destHandle.setValue(destOffset, blended);
                break;
            }
            case attr::AttributeType::boolT:
            {
                // Booleans don't blend so fall back to the nearest endpoint.
                attr::AttributeHandleRO<boolT> sourceHandle(sourceAttribute);
                attr::AttributeHandle<boolT> destHandle(destAttribute);
                const Offset chosen = (parametricT < 0.5) ? endpointOffset0 : endpointOffset1;
                destHandle.setValue(destOffset, sourceHandle.getValue(chosen));
                break;
            }
            case attr::AttributeType::matrixT:
            {
                // Matrices don't blend componentwise meaningfully so pick an endpoint.
                attr::AttributeHandleRO<Matrix4> sourceHandle(sourceAttribute);
                attr::AttributeHandle<Matrix4> destHandle(destAttribute);
                const Offset chosen = (parametricT < 0.5) ? endpointOffset0 : endpointOffset1;
                destHandle.setValue(destOffset, sourceHandle.getValue(chosen));
                break;
            }
            default: break;
        }
    }
}


// Build a new attribute on the destination mesh shaped like a source attribute
// from A or B. Skips intrinsics and attributes that already exist.
void ensureAttributeOnDestination(geo::Mesh& destMesh,
                                  attr::AttributeOwner owner,
                                  const std::shared_ptr<attr::Attribute>& sourceAttribute)
{
    if (!sourceAttribute) return;
    if (sourceAttribute->isIntrinsic()) return;
    if (destMesh.attributeExists(owner, sourceAttribute->getName())) return;

    const std::string name = sourceAttribute->getName();
    switch (sourceAttribute->getType())
    {
        case attr::AttributeType::intT:    destMesh.addIntAttribute(owner, name); break;
        case attr::AttributeType::floatT:  /* no addFloatAttribute helper, fall through */ break;
        case attr::AttributeType::vectorT: destMesh.addVector3Attribute(owner, name); break;
        case attr::AttributeType::boolT:   destMesh.addBoolAttribute(owner, name, false, sourceAttribute->isPrivate()); break;
        case attr::AttributeType::matrixT: destMesh.addMatrix4Attribute(owner, name); break;
        default: break;
    }
}


// Assemble the output mesh from fragments and detriangulated polygons. Points
// are laid out as A-survivors first, then B-survivors, then cut points, then
// any fully new points. The point remap lets us rewrite each loop's corners
// in terms of the new mesh's point offsets.
std::shared_ptr<geo::Mesh> assembleMesh(const geo::Mesh& meshA,
                                         const geo::Mesh& meshB,
                                         const BooleanFragments& fragments,
                                         const std::vector<VertexOrigin>& origins,
                                         const std::vector<DetriangulatedFace>& faces)
{
    auto outputMesh = std::make_shared<geo::Mesh>(meshA.getPath());

    // Decide which fragment vertices are actually used by the surviving polygons.
    std::vector<bool> vertexUsed(fragments.positions.size(), false);
    for (const DetriangulatedFace& face : faces)
        for (int vertIndex : face.loop)
            vertexUsed[vertIndex] = true;

    // Sort used vertices into ordered buckets. A originals first, then B originals, then anything else.
    std::vector<int> aOriginalVerts;
    std::vector<int> bOriginalVerts;
    std::vector<int> otherVerts;
    for (size_t vertIndex = 0; vertIndex < origins.size(); ++vertIndex)
    {
        if (!vertexUsed[vertIndex]) continue;
        const VertexOrigin& origin = origins[vertIndex];
        if (origin.kind == VertexOrigin::Kind::ORIGINAL && origin.side == VertexOrigin::Side::A)
            aOriginalVerts.push_back(static_cast<int>(vertIndex));
        else if (origin.kind == VertexOrigin::Kind::ORIGINAL && origin.side == VertexOrigin::Side::B)
            bOriginalVerts.push_back(static_cast<int>(vertIndex));
        else
            otherVerts.push_back(static_cast<int>(vertIndex));
    }
    // Keep A originals in source point order, then B originals in source point order.
    auto byOriginalPointOffset = [&](int leftVert, int rightVert)
    {
        return origins[leftVert].endpoint0 < origins[rightVert].endpoint0;
    };
    std::sort(aOriginalVerts.begin(), aOriginalVerts.end(), byOriginalPointOffset);
    std::sort(bOriginalVerts.begin(), bOriginalVerts.end(), byOriginalPointOffset);

    // Add points in order and remember each fragment vertex's new offset.
    std::vector<Offset> fragmentToOutputPoint(fragments.positions.size(),
                                                   static_cast<Offset>(-1));
    std::vector<Vector3> orderedPositions;
    orderedPositions.reserve(aOriginalVerts.size() + bOriginalVerts.size() + otherVerts.size());
    auto appendVertex = [&](int fragmentVertex)
    {
        fragmentToOutputPoint[fragmentVertex] = orderedPositions.size();
        orderedPositions.push_back(fragments.positions[fragmentVertex]);
    };
    for (int vertIndex : aOriginalVerts) appendVertex(vertIndex);
    for (int vertIndex : bOriginalVerts) appendVertex(vertIndex);
    for (int vertIndex : otherVerts)     appendVertex(vertIndex);
    outputMesh->addPoints(orderedPositions);

    // Mirror every non intrinsic attribute schema from A and B onto the output
    // mesh so the copy step below has a destination column to write into.
    auto mirrorAttributes = [&](const geo::Mesh& sourceMesh, attr::AttributeOwner owner)
    {
        const size_t count = sourceMesh.getNumAttributes(owner);
        for (size_t attrIndex = 0; attrIndex < count; ++attrIndex)
        {
            auto weak = sourceMesh.getAttributeByIndex(owner, static_cast<unsigned int>(attrIndex));
            auto shared = weak.lock();
            if (!shared) continue;
            // weak holds shared_ptr<const Attribute> so cast away const to fit the helper signature.
            auto mutableShared = std::const_pointer_cast<attr::Attribute>(shared);
            ensureAttributeOnDestination(*outputMesh, owner, mutableShared);
        }
    };
    mirrorAttributes(meshA, attr::AttributeOwner::POINT);
    mirrorAttributes(meshB, attr::AttributeOwner::POINT);
    mirrorAttributes(meshA, attr::AttributeOwner::VERTEX);
    mirrorAttributes(meshB, attr::AttributeOwner::VERTEX);
    mirrorAttributes(meshA, attr::AttributeOwner::FACE);
    mirrorAttributes(meshB, attr::AttributeOwner::FACE);

    // Copy point attributes over each output point, interpolating cut points.
    auto copyPointAttrFor = [&](int fragmentVertex, Offset destPointOffset)
    {
        const VertexOrigin& origin = origins[fragmentVertex];
        if (origin.kind == VertexOrigin::Kind::ORIGINAL)
        {
            const geo::Mesh& sourceMesh = (origin.side == VertexOrigin::Side::A) ? meshA : meshB;
            // We can't reach a protected attrib store here, so iterate by name.
            const size_t count = sourceMesh.getNumAttributes(attr::AttributeOwner::POINT);
            for (size_t attrIndex = 0; attrIndex < count; ++attrIndex)
            {
                auto sourceWeak = sourceMesh.getAttributeByIndex(attr::AttributeOwner::POINT,
                                                                  static_cast<unsigned int>(attrIndex));
                auto sourceShared = sourceWeak.lock();
                if (!sourceShared) continue;
                auto destShared = outputMesh->getAttribByName(attr::AttributeOwner::POINT,
                                                               sourceShared->getName());
                if (!destShared) continue;
                if (destShared->getType() != sourceShared->getType()) continue;
                attr::attribVector singleSource = { std::const_pointer_cast<attr::Attribute>(sourceShared) };
                attr::attribVector singleDest = { destShared };
                copyAttributeRow(singleSource, singleDest, origin.endpoint0, destPointOffset);
            }
        }
        else if (origin.kind == VertexOrigin::Kind::ON_EDGE)
        {
            const geo::Mesh& sourceMesh = (origin.side == VertexOrigin::Side::A) ? meshA : meshB;
            const size_t count = sourceMesh.getNumAttributes(attr::AttributeOwner::POINT);
            for (size_t attrIndex = 0; attrIndex < count; ++attrIndex)
            {
                auto sourceWeak = sourceMesh.getAttributeByIndex(attr::AttributeOwner::POINT,
                                                                  static_cast<unsigned int>(attrIndex));
                auto sourceShared = sourceWeak.lock();
                if (!sourceShared) continue;
                auto destShared = outputMesh->getAttribByName(attr::AttributeOwner::POINT,
                                                               sourceShared->getName());
                if (!destShared) continue;
                if (destShared->getType() != sourceShared->getType()) continue;
                attr::attribVector singleSource = { std::const_pointer_cast<attr::Attribute>(sourceShared) };
                attr::attribVector singleDest = { destShared };
                interpolateAttributeRow(singleSource, singleDest,
                                         origin.endpoint0, origin.endpoint1, origin.t,
                                         destPointOffset);
            }
        }
        // NEW points keep their default initialization.
    };
    for (size_t fragmentVertex = 0; fragmentVertex < fragments.positions.size(); ++fragmentVertex)
    {
        const Offset destPointOffset = fragmentToOutputPoint[fragmentVertex];
        if (destPointOffset == static_cast<Offset>(-1)) continue;
        copyPointAttrFor(static_cast<int>(fragmentVertex), destPointOffset);
    }

    // Sort detriangulated faces so A faces come first in source order, then B.
    std::vector<size_t> faceOrder(faces.size());
    for (size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) faceOrder[faceIndex] = faceIndex;
    std::sort(faceOrder.begin(), faceOrder.end(), [&](size_t leftIndex, size_t rightIndex)
    {
        const auto leftSide = static_cast<int>(faces[leftIndex].source.side);
        const auto rightSide = static_cast<int>(faces[rightIndex].source.side);
        if (leftSide != rightSide) return leftSide < rightSide;
        return faces[leftIndex].source.faceOffset < faces[rightIndex].source.faceOffset;
    });

    // Emit faces and remember the order so face attrs line up.
    std::vector<Offset> flatPointOffsets;
    std::vector<Offset> vertexCounts;
    for (size_t orderedIndex : faceOrder)
    {
        const DetriangulatedFace& face = faces[orderedIndex];
        for (int vertIndex : face.loop)
            flatPointOffsets.push_back(fragmentToOutputPoint[vertIndex]);
        vertexCounts.push_back(static_cast<Offset>(face.loop.size()));
    }
    outputMesh->addFaces(flatPointOffsets, vertexCounts);

    // Copy face and vertex attributes by walking output faces in the order they were added.
    Offset cursorFaceOffset = 0;
    Offset cursorVertexOffset = 0;
    for (size_t orderedIndex : faceOrder)
    {
        const DetriangulatedFace& face = faces[orderedIndex];
        const geo::Mesh& sourceMesh = (face.source.side == SourceFace::Side::A) ? meshA : meshB;

        // Copy face attrs from source face to the new face.
        const size_t faceAttrCount = sourceMesh.getNumAttributes(attr::AttributeOwner::FACE);
        for (size_t attrIndex = 0; attrIndex < faceAttrCount; ++attrIndex)
        {
            auto sourceWeak = sourceMesh.getAttributeByIndex(attr::AttributeOwner::FACE,
                                                              static_cast<unsigned int>(attrIndex));
            auto sourceShared = sourceWeak.lock();
            if (!sourceShared) continue;
            auto destShared = outputMesh->getAttribByName(attr::AttributeOwner::FACE,
                                                           sourceShared->getName());
            if (!destShared) continue;
            if (destShared->getType() != sourceShared->getType()) continue;
            attr::attribVector singleSource = { std::const_pointer_cast<attr::Attribute>(sourceShared) };
            attr::attribVector singleDest = { destShared };
            copyAttributeRow(singleSource, singleDest, face.source.faceOffset, cursorFaceOffset);
        }

        // Match each output corner back to a source vertex on the source face
        // by walking the source face's corners and comparing point offsets.
        const std::span<const intT> sourceFacePoints = sourceMesh.getFacePoints(face.source.faceOffset);
        const Offset sourceFaceStart = sourceMesh.getFaceStartVertex(face.source.faceOffset);

        for (size_t cornerIndex = 0; cornerIndex < face.loop.size(); ++cornerIndex)
        {
            const int fragmentVertex = face.loop[cornerIndex];
            const VertexOrigin& origin = origins[fragmentVertex];
            const Offset destVertexOffset = cursorVertexOffset + cornerIndex;

            // Only proceed if this corner came from a point of the same source face.
            if (origin.kind != VertexOrigin::Kind::ORIGINAL) continue;
            const VertexOrigin::Side originSide = origin.side;
            const auto sourceSideEnum = (face.source.side == SourceFace::Side::A) ? VertexOrigin::Side::A : VertexOrigin::Side::B;
            if (originSide != sourceSideEnum) continue;

            // Find the matching source vertex offset by scanning source face corners.
            Offset matchedSourceVertex = static_cast<Offset>(-1);
            for (size_t sourceCorner = 0; sourceCorner < sourceFacePoints.size(); ++sourceCorner)
            {
                if (static_cast<Offset>(sourceFacePoints[sourceCorner]) == origin.endpoint0)
                {
                    matchedSourceVertex = sourceFaceStart + sourceCorner;
                    break;
                }
            }
            if (matchedSourceVertex == static_cast<Offset>(-1)) continue;

            const size_t vertexAttrCount = sourceMesh.getNumAttributes(attr::AttributeOwner::VERTEX);
            for (size_t attrIndex = 0; attrIndex < vertexAttrCount; ++attrIndex)
            {
                auto sourceWeak = sourceMesh.getAttributeByIndex(attr::AttributeOwner::VERTEX,
                                                                  static_cast<unsigned int>(attrIndex));
                auto sourceShared = sourceWeak.lock();
                if (!sourceShared) continue;
                auto destShared = outputMesh->getAttribByName(attr::AttributeOwner::VERTEX,
                                                               sourceShared->getName());
                if (!destShared) continue;
                if (destShared->getType() != sourceShared->getType()) continue;
                attr::attribVector singleSource = { std::const_pointer_cast<attr::Attribute>(sourceShared) };
                attr::attribVector singleDest = { destShared };
                copyAttributeRow(singleSource, singleDest, matchedSourceVertex, destVertexOffset);
            }
        }

        cursorVertexOffset += face.loop.size();
        ++cursorFaceOffset;
    }

    return outputMesh;
}

} // namespace


std::shared_ptr<geo::Mesh> booleanMesh(const geo::Mesh& meshA,
                                       const geo::Mesh& meshB,
                                       BooleanOp op,
                                       std::string* error)
{
    // Run Manifold and convert the result into our backend neutral form.
    const BooleanFragments fragments = runManifold(meshA, meshB, op, error);

    // Tag each fragment vertex by where it came from in A or B.
    const std::vector<VertexOrigin> origins = resolveOrigins(fragments, meshA, meshB);

    // Walk every source face's triangle bucket back into a polygon.
    const std::vector<DetriangulatedFace> faces = detriangulate(fragments, error);

    // Build the output mesh and carry attributes from inputs onto it.
    return assembleMesh(meshA, meshB, fragments, origins, faces);
}

} // namespace enzo::utils
