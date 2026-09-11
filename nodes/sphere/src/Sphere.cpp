#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

namespace {

// The rim points of one ring of latitude, plus the point on the axis that a
// sealed arc's walls anchor to.
struct Ring
{
    std::vector<enzo::Offset> rim;
    enzo::Offset axisPoint = 0;
};

// The shape of the sphere, read once from the parameters.
struct SphereSettings
{
    int columns = 16;
    int rows = 8;
    enzo::Vector3 radius = {1, 1, 1};
    float arcBegin = 0;
    float arcSpan = 1;
    bool isArc = false;
    bool sealArc = false;
};

// Returns the rotation in degrees that puts the poles on the named axis. The
// rings are built around Y, so Y itself needs no rotation.
enzo::Vector3 getAxisRotation(const std::string& axis)
{
    if (axis == "x") return {0, 0, -90};
    if (axis == "z") return {90, 0, 0};
    return {0, 0, 0};
}

// Adds one ring of latitude, at 0 for the bottom pole and at 1 for the top.
Ring addRing(enzo::geo::Mesh& mesh, const SphereSettings& settings, float heightFraction)
{
    const float latitude = heightFraction * std::numbers::pi_v<float>;
    const float ringScale = std::sin(latitude);
    const float ringY = -std::cos(latitude) * settings.radius.y();

    // A closed ring joins its last point back to its first, so it stops one
    // step short of the full turn.
    const int rimPointCount = settings.columns + settings.isArc;

    std::vector<enzo::Vector3> positions;
    positions.reserve(rimPointCount);
    for (int columnIndex = 0; columnIndex < rimPointCount; ++columnIndex)
    {
        const float turn = settings.arcBegin + settings.arcSpan * columnIndex / settings.columns;
        const float angle = turn * std::numbers::pi_v<float> * 2;
        positions.push_back(
            {std::sin(angle) * ringScale * settings.radius.x(),
             ringY,
             std::cos(angle) * ringScale * settings.radius.z()}
        );
    }

    Ring ring;
    ring.rim = mesh.addPoints(positions);
    if (settings.sealArc) ring.axisPoint = mesh.addPoint({0, ringY, 0});
    return ring;
}

// Adds the band of quads running around the surface between two rings.
void addSideFaces(enzo::geo::Mesh& mesh, const Ring& lower, const Ring& upper, int columns)
{
    const size_t rimPointCount = lower.rim.size();
    for (int columnIndex = 0; columnIndex < columns; ++columnIndex)
    {
        const size_t nextColumn = (columnIndex + 1) % rimPointCount;
        mesh.addFace(
            {lower.rim[columnIndex],
             lower.rim[nextColumn],
             upper.rim[nextColumn],
             upper.rim[columnIndex]}
        );
    }
}

// Adds the two flat walls running from the axis to the rim that seal the open
// sides of a closed arc.
void addArcWalls(enzo::geo::Mesh& mesh, const Ring& lower, const Ring& upper)
{
    mesh.addFace({lower.axisPoint, lower.rim.front(), upper.rim.front(), upper.axisPoint});
    mesh.addFace({lower.rim.back(), lower.axisPoint, upper.axisPoint, upper.rim.back()});
}

// Adds one triangle at a pole, wound so that it faces away from the centre.
void addPoleFace(enzo::geo::Mesh& mesh, std::vector<enzo::Offset> points, bool isTop)
{
    if (!isTop) std::reverse(points.begin(), points.end());
    mesh.addFace(points);
}

// Adds the fan of triangles joining a ring to the pole beyond it, along with
// the two walls that seal a closed arc there.
void addPoleFaces(
    enzo::geo::Mesh& mesh,
    const SphereSettings& settings,
    const Ring& ring,
    enzo::Offset pole,
    bool isTop
)
{
    const size_t rimPointCount = ring.rim.size();
    for (int columnIndex = 0; columnIndex < settings.columns; ++columnIndex)
    {
        const size_t nextColumn = (columnIndex + 1) % rimPointCount;
        addPoleFace(mesh, {ring.rim[columnIndex], ring.rim[nextColumn], pole}, isTop);
    }

    if (settings.sealArc)
    {
        addPoleFace(mesh, {ring.axisPoint, ring.rim.front(), pole}, isTop);
        addPoleFace(mesh, {ring.rim.back(), ring.axisPoint, pole}, isTop);
    }
}

class Sphere : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;

  private:
    SphereSettings readSettings() const;
};

SphereSettings Sphere::readSettings() const
{
    const std::string arcType = evalParmString("arc");

    SphereSettings settings;
    settings.columns = evalParmInt("columns");
    settings.rows = evalParmInt("rows");
    settings.radius = evalParmVector3("radius");
    settings.isArc = arcType != "closed";
    settings.sealArc = arcType == "closed_arc";

    if (settings.isArc)
    {
        settings.arcBegin = evalParmFloat("arcAngles", 0) / 360;
        settings.arcSpan = evalParmFloat("arcAngles", 1) / 360 - settings.arcBegin;
    }

    return settings;
}

void Sphere::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();
    const SphereSettings settings = readSettings();

    const Offset bottomPole = mesh->addPoint({0, -settings.radius.y(), 0});

    // Builds a ring at each row boundary, with the poles standing in for the
    // rings at the two ends.
    std::vector<Ring> rings;
    rings.reserve(settings.rows - 1);
    for (int rowIndex = 1; rowIndex < settings.rows; ++rowIndex)
        rings.push_back(
            addRing(*mesh, settings, static_cast<float>(rowIndex) / settings.rows)
        );

    const Offset topPole = mesh->addPoint({0, settings.radius.y(), 0});

    constexpr bool isBottomPole = false;
    constexpr bool isTopPole = true;

    addPoleFaces(*mesh, settings, rings.front(), bottomPole, isBottomPole);
    for (size_t ringIndex = 0; ringIndex + 1 < rings.size(); ++ringIndex)
    {
        addSideFaces(*mesh, rings[ringIndex], rings[ringIndex + 1], settings.columns);
        if (settings.sealArc) addArcWalls(*mesh, rings[ringIndex], rings[ringIndex + 1]);
    }
    addPoleFaces(*mesh, settings, rings.back(), topPole, isTopPole);

    mesh->applyTransform(Transform()
                             .translate(evalParmVector3("center"))
                             .rotateEuler(evalParmVector3("rotate"))
                             .rotateEuler(getAxisRotation(evalParmString("axis")))
                             .scale(evalParmFloat("uniformScale")));

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(sphere, Sphere)
