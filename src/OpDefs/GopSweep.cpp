#include "OpDefs/GopSweep.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <cmath>
#include <numbers>

namespace {
using namespace enzo;

/**
 * @brief Builds a closed unit circle profile in the XY plane.
 *
 * The circle's normal runs along local Z so lookAt can aim it down a curve
 * tangent. Points stop one short of a full turn so the last wraps back onto
 * the first.
 */
std::vector<Vector3> buildCircleProfile(int pointCount)
{
    std::vector<Vector3> profile;
    profile.reserve(pointCount);
    for (int i = 0; i < pointCount; ++i)
    {
        const float angle = static_cast<float>(i) / pointCount * 2 * std::numbers::pi;
        profile.push_back({std::sin(angle), std::cos(angle), 0});
    }
    return profile;
}

/**
 * @brief A sweep of a profile along every curve of one mesh.
 *
 * Holds the mesh and the buffers the sweep fills so each curve is handled by
 * reading members rather than threading state through call after call.
 */
class Sweeper
{
  public:
    Sweeper(geo::Mesh& mesh, std::span<const Vector3> profile)
        : mesh_(mesh), profile_(profile), profilePointCount_(static_cast<int>(profile.size())),
          faceTangents_(mesh, utils::TangentMode::TwoPoint), faceNormals_(mesh.getFaceNormal())
    {
    }

    /// @brief Replaces every curve with a tube surface swept from the profile.
    void sweep()
    {
        const std::vector<Offset> sourceCurves = mesh_.getFaces().toVector();

        // Sweep the profile along every curve, gathering all ring points for one
        // batched insert.
        ringPositions_.reserve(profilePointCount_ * mesh_.getNumPoints());
        for (Offset curveFace : sourceCurves)
            if (mesh_.getFacePointCount(curveFace) >= 2) addRings(curveFace);
        ringPoints_ = mesh_.addPoints(ringPositions_);

        // Bridge the rings into quad faces, walking the curves in the order their
        // rings were laid down so ringStart stays in step.
        size_t ringStart = 0;
        for (Offset curveFace : sourceCurves)
        {
            const int curvePointCount = mesh_.getFacePointCount(curveFace);
            if (curvePointCount < 2) continue;
            bridgeRings(curveFace, ringStart);
            ringStart += curvePointCount * profilePointCount_;
        }
        mesh_.addFaces(facePointOffsets_, faceVertexCounts_);

        // Remove the source curves now the swept surface replaces them.
        mesh_.deleteFaces(sourceCurves);
    }

  private:
    /// @brief Adds a ring of profile points at every point along the curve.
    void addRings(Offset curveFace)
    {
        const int curvePointCount = mesh_.getFacePointCount(curveFace);
        const std::span<const Vector3> tangents = faceTangents_(curveFace);
        const Vector3 normal = faceNormals_[curveFace];

        int relPointNum = 0;
        for (intT pointOffset : mesh_.getFacePoints(curveFace))
        {
            const float curveU = static_cast<float>(relPointNum) / (curvePointCount - 1);

            // The scale runs first so it sizes the profile before the orientation
            // turns it.
            Transform transform =
                Transform::lookAt(mesh_.getPointPos(pointOffset), tangents[relPointNum], normal);
            transform.scale(curveU); // TODO: control radius with a ramp parameter

            for (const Vector3& profilePos : profile_)
                ringPositions_.push_back(transform * profilePos);

            ++relPointNum;
        }
    }

    /// @brief Bridges the curve's neighbouring rings into quad faces.
    void bridgeRings(Offset curveFace, size_t ringStart)
    {
        const int curvePointCount = mesh_.getFacePointCount(curveFace);

        // The profile wraps its last point to its first, and a closed curve wraps
        // its last ring back to its first.
        const int bridgeCount = mesh_.isClosed(curveFace) ? curvePointCount : curvePointCount - 1;
        for (int ring = 0; ring < bridgeCount; ++ring)
        {
            const size_t ringA = ringStart + ring * profilePointCount_;
            const size_t ringB = ringStart + ((ring + 1) % curvePointCount) * profilePointCount_;

            for (int j = 0; j < profilePointCount_; ++j)
            {
                const int nextJ = (j + 1) % profilePointCount_;

                facePointOffsets_.push_back(ringPoints_[ringA + j]);
                facePointOffsets_.push_back(ringPoints_[ringA + nextJ]);
                facePointOffsets_.push_back(ringPoints_[ringB + nextJ]);
                facePointOffsets_.push_back(ringPoints_[ringB + j]);
                faceVertexCounts_.push_back(4);
            }
        }
    }

    geo::Mesh& mesh_;
    std::span<const Vector3> profile_;
    int profilePointCount_;
    utils::FaceTangents faceTangents_;
    geo::FaceNormalHandle faceNormals_;

    std::vector<Vector3> ringPositions_;
    std::vector<Offset> ringPoints_;
    std::vector<Offset> facePointOffsets_;
    std::vector<Offset> faceVertexCounts_;
};

} // namespace

GopSweep::GopSweep(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopSweep::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = context.cloneInputPacket(0);

    const std::vector<Vector3> profile = buildCircleProfile(10);

    for (auto prim : packet.getPrimitives())
    {
        std::shared_ptr<geo::Mesh> mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
        if (!mesh) continue;

        Sweeper(*mesh, profile).sweep();
    }

    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopSweep::parameterList() { return {}; }
