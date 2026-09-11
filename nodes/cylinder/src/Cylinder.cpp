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

// The rim points of one horizontal ring, plus the point on the axis that a
// sealed arc's walls and caps anchor to.
struct Ring
{
    std::vector<enzo::Offset> rim;
    enzo::Offset axisPoint = 0;
};

// The shape of the cylinder, read once from the parameters.
struct CylinderSettings
{
    int columns = 12;
    int rows = 1;
    float bottomRadius = 1;
    float topRadius = 1;
    float height = 2;
    float arcBegin = 0;
    float arcSpan = 1;
    bool isArc = false;
    bool sealArc = false;
    bool capEnds = true;
    bool weldCaps = true;
    std::string capGroupName;
};

// Returns the rotation in degrees that stands the cylinder up along the named
// axis. The rings are built along Y, so Y itself needs no rotation.
enzo::Vector3 getAxisRotation(const std::string& axis)
{
    if (axis == "x") return {0, 0, -90};
    if (axis == "z") return {90, 0, 0};
    return {0, 0, 0};
}

// Adds one horizontal ring of points, at 0 for the bottom of the cylinder and
// at 1 for the top.
Ring addRing(enzo::geo::Mesh& mesh, const CylinderSettings& settings, float heightFraction)
{
    const float radius = std::lerp(settings.bottomRadius, settings.topRadius, heightFraction);
    const float ringY = (heightFraction - 0.5f) * settings.height;

    // A closed ring joins its last point back to its first, so it stops one
    // step short of the full turn.
    const int rimPointCount = settings.columns + settings.isArc;

    std::vector<enzo::Vector3> positions;
    positions.reserve(rimPointCount);
    for (int columnIndex = 0; columnIndex < rimPointCount; ++columnIndex)
    {
        const float turn =
            settings.arcBegin + settings.arcSpan * columnIndex / settings.columns;
        const float angle = turn * std::numbers::pi_v<float> * 2;
        positions.push_back({std::sin(angle) * radius, ringY, std::cos(angle) * radius});
    }

    Ring ring;
    ring.rim = mesh.addPoints(positions);
    if (settings.sealArc) ring.axisPoint = mesh.addPoint({0, ringY, 0});
    return ring;
}

// Adds the band of quads running around the side between two rings.
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

/// @brief Adds the polygon closing one end of the cylinder.
/// @return The offset of the cap face.
/// @note An unwelded cap gets its own copy of the rim, leaving the edge between
/// the cap and the side sharp.
enzo::Offset addCap(
    enzo::geo::Mesh& mesh,
    const CylinderSettings& settings,
    const Ring& rimRing,
    float heightFraction,
    bool isTop
)
{
    const Ring capRing = settings.weldCaps ? rimRing : addRing(mesh, settings, heightFraction);

    std::vector<enzo::Offset> capPoints = capRing.rim;
    if (settings.sealArc) capPoints.push_back(capRing.axisPoint);

    // The rims run the same way round at both ends, so the top has to be
    // reversed for the two caps to face away from each other.
    if (isTop) std::reverse(capPoints.begin(), capPoints.end());

    return mesh.addFace(capPoints);
}

class Cylinder : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;

  private:
    CylinderSettings readSettings() const;
};

CylinderSettings Cylinder::readSettings() const
{
    const std::string arcType = evalParmString("arc");

    CylinderSettings settings;
    settings.columns = evalParmInt("columns");
    settings.rows = evalParmInt("rows");
    const float radiusMultiplier = evalParmFloat("uniformRadius");
    settings.topRadius = evalParmFloat("radius", 0) * radiusMultiplier;
    settings.bottomRadius = evalParmFloat("radius", 1) * radiusMultiplier;
    settings.height = evalParmFloat("height");
    settings.isArc = arcType != "closed";
    settings.sealArc = arcType == "closed_arc";
    settings.capEnds = evalParmString("endCapType") == "single";
    settings.weldCaps = evalParmBool("weldCaps");

    if (settings.isArc)
    {
        settings.arcBegin = evalParmFloat("arcAngles", 0) / 360;
        settings.arcSpan = evalParmFloat("arcAngles", 1) / 360 - settings.arcBegin;
    }
    if (settings.capEnds && evalParmBool("endCapGroupEnabled"))
        settings.capGroupName = evalParmString("endCapGroupName");

    return settings;
}

void Cylinder::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();
    const CylinderSettings settings = readSettings();

    std::vector<Ring> rings;
    rings.reserve(settings.rows + 1);
    for (int rowIndex = 0; rowIndex <= settings.rows; ++rowIndex)
        rings.push_back(
            addRing(*mesh, settings, static_cast<float>(rowIndex) / settings.rows)
        );

    for (int rowIndex = 0; rowIndex < settings.rows; ++rowIndex)
    {
        addSideFaces(*mesh, rings[rowIndex], rings[rowIndex + 1], settings.columns);
        if (settings.sealArc) addArcWalls(*mesh, rings[rowIndex], rings[rowIndex + 1]);
    }

    // A cap on an end that has come to a point would be a face with no area.
    std::vector<Offset> capFaces;
    if (settings.capEnds && settings.bottomRadius > 0)
    {
        constexpr float bottomHeightFraction = 0;
        constexpr bool isTopCap = false;
        capFaces.push_back(addCap(*mesh, settings, rings.front(), bottomHeightFraction, isTopCap));
    }
    if (settings.capEnds && settings.topRadius > 0)
    {
        constexpr float topHeightFraction = 1;
        constexpr bool isTopCap = true;
        capFaces.push_back(addCap(*mesh, settings, rings.back(), topHeightFraction, isTopCap));
    }

    if (!settings.capGroupName.empty() && !capFaces.empty())
    {
        mesh->addFaceGroup(settings.capGroupName);
        mesh->addToFaceGroup(settings.capGroupName, capFaces);
    }

    mesh->applyTransform(Transform()
                             .translate(evalParmVector3("center"))
                             .rotateEuler(evalParmVector3("rotate"))
                             .rotateEuler(getAxisRotation(evalParmString("axis")))
                             .scale(evalParmFloat("uniformScale")));

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(cylinder, Cylinder)
