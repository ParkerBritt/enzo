#include "OpDefs/GopSweep.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <cmath>
#include <numbers>

GopSweep::GopSweep(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopSweep::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = context.cloneInputPacket(0);

    // Profile swept along every curve. A closed unit circle in the XY plane whose
    // normal runs along local Z so lookAt can aim it down a curve tangent. Points
    // stop one short of a full turn so the last wraps back onto the first.
    constexpr int profilePointCount = 10;
    std::vector<Vector3> profilePositions;
    profilePositions.reserve(profilePointCount);
    for (int i = 0; i < profilePointCount; ++i)
    {
        const float angle = static_cast<float>(i) / profilePointCount * 2 * std::numbers::pi;
        profilePositions.push_back({std::sin(angle), std::cos(angle), 0});
    }

    for (auto prim : packet.getPrimitives())
    {
        std::shared_ptr<geo::Mesh> mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
        if (!mesh) continue;

        const std::vector<Offset> sourceCurves = mesh->getFaces().toVector();
        utils::FaceTangents faceTangents(*mesh, utils::TangentMode::TwoPoint);
        geo::FaceNormalHandle faceNormals = mesh->getFaceNormal();

        // Place a ring of profile points at every point along every curve, then
        // add them all in one batch. The rings keep curve order so the second
        // pass walks the same curves to find them.
        std::vector<Vector3> ringPositions;
        ringPositions.reserve(profilePointCount * mesh->getNumPoints());
        for (Offset curveFace : sourceCurves)
        {
            const int curvePointCount = mesh->getFacePointCount(curveFace);
            if (curvePointCount < 2) continue;

            const std::span<const Vector3> tangents = faceTangents(curveFace);
            const Vector3 normal = faceNormals[curveFace];

            int relPointNum = 0;
            for (intT pointOffset : mesh->getFacePoints(curveFace))
            {
                const float curveU = static_cast<float>(relPointNum) / (curvePointCount - 1);

                // Aim the profile down the tangent at the curve point and scale
                // its radius. The scale runs first so it sizes the profile before
                // the orientation turns it.
                Transform transform =
                    Transform::lookAt(mesh->getPointPos(pointOffset), tangents[relPointNum], normal);
                transform.scale(curveU); // TODO: control radius with a ramp parameter

                for (const Vector3& profilePos : profilePositions)
                    ringPositions.push_back(transform * profilePos);

                ++relPointNum;
            }
        }
        const std::vector<Offset> ringPoints = mesh->addPoints(ringPositions);

        // Bridge a quad between neighbouring rings and neighbouring profile
        // points. The profile wraps its last point to its first, and a closed
        // curve wraps its last ring back to its first. ringStart walks the rings
        // in step with the curves they came from.
        std::vector<Offset> facePointOffsets;
        std::vector<Offset> faceVertexCounts;
        size_t ringStart = 0;
        for (Offset curveFace : sourceCurves)
        {
            const int curvePointCount = mesh->getFacePointCount(curveFace);
            if (curvePointCount < 2) continue;

            const int bridgeCount =
                mesh->isClosed(curveFace) ? curvePointCount : curvePointCount - 1;
            for (int ring = 0; ring < bridgeCount; ++ring)
            {
                const size_t ringA = ringStart + ring * profilePointCount;
                const size_t ringB = ringStart + ((ring + 1) % curvePointCount) * profilePointCount;

                for (int j = 0; j < profilePointCount; ++j)
                {
                    const int nextJ = (j + 1) % profilePointCount;

                    facePointOffsets.push_back(ringPoints[ringA + j]);
                    facePointOffsets.push_back(ringPoints[ringA + nextJ]);
                    facePointOffsets.push_back(ringPoints[ringB + nextJ]);
                    facePointOffsets.push_back(ringPoints[ringB + j]);
                    faceVertexCounts.push_back(4);
                }
            }
            ringStart += curvePointCount * profilePointCount;
        }
        mesh->addFaces(facePointOffsets, faceVertexCounts);

        // Remove the source curves now the swept surface replaces them.
        mesh->deleteFaces(sourceCurves);
    }

    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopSweep::parameterList() { return {}; }
