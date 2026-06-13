#include "OpDefs/GopSweep.h"
#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshUtils.h"
#include "Engine/Parameter/Ramp.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <cmath>
#include <numbers>
#include <span>

namespace {
// A profile cross section and whether its last point joins back to its first.
// A circle or square is closed, a ribbon or an open input curve is not.
struct Profile
{
    std::vector<enzo::Vector3> points;
    bool closed = true;
};

// Defined at the bottom of the file, below the node code they serve.
Profile buildProfile(enzo::op::Context& context);
std::vector<enzo::Vector3> buildCircleProfile(int columns);
std::vector<enzo::Vector3> buildSquareProfile(int columns);
std::vector<enzo::Vector3> buildRibbonProfile(int columns);
Profile buildProfileFromInput(enzo::NodePacket& packet);
void sweepMesh(
    enzo::geo::Mesh& mesh,
    const Profile& profile,
    bool applyScale,
    const enzo::prm::Ramp& scaleRamp
);
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

    const bool applyScale = context.evalParmBool("applyscale");
    const prm::Ramp scaleRamp = context.evalParmRamp("scaleramp");

    if (context.evalParmString("profileshape") == "input" && !context.hasInput(1))
    {
        throwError("Sweep needs a profile shape wired into the second input.");
        return;
    }
    const Profile profile = buildProfile(context);

    for (auto prim : packet.getPrimitives())
    {
        std::shared_ptr<geo::Mesh> mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
        if (!mesh) continue;

        sweepMesh(*mesh, profile, applyScale, scaleRamp);
    }

    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopSweep::parameterList()
{
    using namespace enzo::prm;
    return {
        Template(Type::DROPDOWN, Name("profileshape", "Profile Shape"), Default("round"))
            .setOptions({
                Name("input", "Second Input"),
                Name("round", "Round"),
                Name("square", "Square"),
                Name("ribbon", "Ribbon"),
            }),
        // One column count per procedural profile, each shown only for its own
        // shape. The condition reads the dropdown's option index, where round is
        // 1, square is 2 and ribbon is 3.
        Template(
            Type::INT,
            Name("roundcolumns", "Columns"),
            Default(12),
            1,
            Range(3, 64, RangeFlag::LOCKED, RangeFlag::UNLOCKED)
        )
            .setDisableWhen("profileshape != 1"),
        Template(
            Type::INT,
            Name("squarecolumns", "Columns"),
            Default(1),
            1,
            Range(1, 64, RangeFlag::LOCKED, RangeFlag::UNLOCKED)
        )
            .setDisableWhen("profileshape != 2"),
        Template(
            Type::INT,
            Name("ribboncolumns", "Columns"),
            Default(2),
            1,
            Range(2, 64, RangeFlag::LOCKED, RangeFlag::UNLOCKED)
        )
            .setDisableWhen("profileshape != 3"),
        Template(Type::BOOL, Name("applyscale", "Apply Scale"), Default(1)),
        Template(Type::RAMP, Name("scaleramp", "Scale Ramp"), Default(2))
            .setInstanceDefault("value", {Default(1), Default(1)})
            .setDisableWhen("applyscale == 0"),
    };
}

namespace {
using namespace enzo;

/**
 * @brief Builds the profile selected by the node's parameters.
 *
 * Each procedural shape reads its own column count. The second input option
 * takes its shape and open or closed state from the wired in curve.
 */
Profile buildProfile(op::Context& context)
{
    const std::string profileShape = context.evalParmString("profileshape");

    if (profileShape == "square")
        return {buildSquareProfile(context.evalParmInt("squarecolumns")), true};
    if (profileShape == "ribbon")
        return {buildRibbonProfile(context.evalParmInt("ribboncolumns")), false};
    if (profileShape == "input")
    {
        NodePacket profilePacket = context.cloneInputPacket(1);
        return buildProfileFromInput(profilePacket);
    }
    return {buildCircleProfile(context.evalParmInt("roundcolumns")), true};
}

/**
 * @brief Builds a closed unit circle profile in the XY plane.
 *
 * The circle's normal runs along local Z so lookAt can aim it down a curve
 * tangent. Points stop one short of a full turn so the last wraps back onto
 * the first.
 */
std::vector<Vector3> buildCircleProfile(int columns)
{
    std::vector<Vector3> profile;
    profile.reserve(columns);
    for (int columnIndex = 0; columnIndex < columns; ++columnIndex)
    {
        const float angle = static_cast<float>(columnIndex) / columns * 2 * std::numbers::pi;
        profile.push_back({std::sin(angle), std::cos(angle), 0});
    }
    return profile;
}

/**
 * @brief Builds a closed unit square profile in the XY plane.
 *
 * Columns set how many segments each side is split into, so the corners always
 * land on points. One column gives a plain square, two adds the edge midpoints.
 */
std::vector<Vector3> buildSquareProfile(int columns)
{
    const Vector3 corners[] = {{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}};

    std::vector<Vector3> profile;
    profile.reserve(columns * 4);
    for (int corner = 0; corner < 4; ++corner)
    {
        // Walk from this corner toward the next, stopping short of it so the
        // next side contributes that shared corner.
        const Vector3& from = corners[corner];
        const Vector3& to = corners[(corner + 1) % 4];
        for (int step = 0; step < columns; ++step)
        {
            const float sideU = static_cast<float>(step) / columns;
            profile.push_back(from + (to - from) * sideU);
        }
    }
    return profile;
}

/**
 * @brief Builds a flat ribbon profile spanning the local X axis.
 *
 * The columns sit evenly from one edge to the other. The profile stays open so
 * the sweep lays a single flat strip rather than a doubled band.
 */
std::vector<Vector3> buildRibbonProfile(int columns)
{
    std::vector<Vector3> profile;
    profile.reserve(columns);
    for (int columnIndex = 0; columnIndex < columns; ++columnIndex)
    {
        const float widthU = static_cast<float>(columnIndex) / (columns - 1);
        profile.push_back({2 * widthU - 1, 0, 0});
    }
    return profile;
}

/**
 * @brief Builds a profile from the first curve of the second input.
 *
 * @return The first mesh face's points and its open or closed state, empty when
 * the input carries no usable face.
 */
Profile buildProfileFromInput(NodePacket& packet)
{
    for (auto prim : packet.getPrimitives())
    {
        std::shared_ptr<geo::Mesh> mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
        if (!mesh || mesh->getNumFaces() == 0) continue;

        Profile profile;
        profile.closed = mesh->isClosed(0);
        for (intT pointOffset : mesh->getFacePoints(0))
            profile.points.push_back(mesh->getPointPos(pointOffset));
        return profile;
    }
    return {};
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
    Sweeper(geo::Mesh& mesh, const Profile& profile, bool applyScale, const prm::Ramp& scaleRamp)
        : mesh_(mesh), profile_(profile),
          profilePointCount_(static_cast<int>(profile.points.size())), applyScale_(applyScale),
          scaleRamp_(scaleRamp), faceTangents_(mesh, utils::TangentMode::TwoPoint),
          faceNormals_(mesh.getFaceNormal())
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
            // The ramp reads how far along the curve the point sits to set the
            // radius. With scaling off the profile keeps its full size.
            const float curveU = static_cast<float>(relPointNum) / (curvePointCount - 1);
            const float radius = applyScale_ ? scaleRamp_.sample(curveU) : 1.0f;

            // The scale runs first so it sizes the profile before the orientation
            // turns it.
            Transform transform =
                Transform::lookAt(mesh_.getPointPos(pointOffset), tangents[relPointNum], normal);
            transform.scale(radius);

            for (const Vector3& profilePos : profile_.points)
                ringPositions_.push_back(transform * profilePos);

            ++relPointNum;
        }
    }

    /// @brief Bridges the curve's neighbouring rings into quad faces.
    void bridgeRings(Offset curveFace, size_t ringStart)
    {
        const int curvePointCount = mesh_.getFacePointCount(curveFace);

        // A closed curve wraps its last ring back to its first, and a closed
        // profile wraps its last column back to its first.
        const int ringBridgeCount =
            mesh_.isClosed(curveFace) ? curvePointCount : curvePointCount - 1;
        const int columnBridgeCount = profile_.closed ? profilePointCount_ : profilePointCount_ - 1;
        for (int ring = 0; ring < ringBridgeCount; ++ring)
        {
            const size_t ringA = ringStart + ring * profilePointCount_;
            const size_t ringB = ringStart + ((ring + 1) % curvePointCount) * profilePointCount_;

            for (int column = 0; column < columnBridgeCount; ++column)
            {
                const int nextColumn = (column + 1) % profilePointCount_;

                facePointOffsets_.push_back(ringPoints_[ringA + column]);
                facePointOffsets_.push_back(ringPoints_[ringA + nextColumn]);
                facePointOffsets_.push_back(ringPoints_[ringB + nextColumn]);
                facePointOffsets_.push_back(ringPoints_[ringB + column]);
                faceVertexCounts_.push_back(4);
            }
        }
    }

    geo::Mesh& mesh_;
    const Profile& profile_;
    int profilePointCount_;
    bool applyScale_;
    const prm::Ramp& scaleRamp_;
    utils::FaceTangents faceTangents_;
    geo::FaceNormalHandle faceNormals_;

    std::vector<Vector3> ringPositions_;
    std::vector<Offset> ringPoints_;
    std::vector<Offset> facePointOffsets_;
    std::vector<Offset> faceVertexCounts_;
};

void sweepMesh(geo::Mesh& mesh, const Profile& profile, bool applyScale, const prm::Ramp& scaleRamp)
{
    Sweeper(mesh, profile, applyScale, scaleRamp).sweep();
}

} // namespace
