#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

namespace {

// The parameters that shape the torus.
struct TorusSettings
{
    int columns = 24;
    int rows = 12;
    float ringRadius = 1;
    float tubeRadius = 0.5f;
};

// Returns the rotation in degrees that puts the hole of the torus on the named
// axis. The torus is built around Y, so Y itself needs no rotation.
enzo::Vector3 getAxisRotation(const std::string& axis)
{
    if (axis == "x") return {0, 0, -90};
    if (axis == "z") return {90, 0, 0};
    return {0, 0, 0};
}

// Adds a ring of points around the tube, columnFraction of the way around the hole.
std::vector<enzo::Offset>
addTubeRing(enzo::geo::Mesh& mesh, const TorusSettings& settings, float columnFraction)
{
    const float ringAngle = columnFraction * std::numbers::pi_v<float> * 2;

    std::vector<enzo::Vector3> positions;
    positions.reserve(settings.rows);
    for (int rowIndex = 0; rowIndex < settings.rows; ++rowIndex)
    {
        const float tubeAngle =
            static_cast<float>(rowIndex) / settings.rows * std::numbers::pi_v<float> * 2;
        const float distanceFromAxis =
            settings.ringRadius + std::cos(tubeAngle) * settings.tubeRadius;
        positions.push_back(
            {std::sin(ringAngle) * distanceFromAxis,
             std::sin(tubeAngle) * settings.tubeRadius,
             std::cos(ringAngle) * distanceFromAxis}
        );
    }

    return mesh.addPoints(positions);
}

// Adds the band of quads going around the tube between two neighbouring rings.
void addBandFaces(
    enzo::geo::Mesh& mesh,
    const std::vector<enzo::Offset>& ring,
    const std::vector<enzo::Offset>& nextRing
)
{
    const size_t rowCount = ring.size();
    for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
    {
        const size_t nextRow = (rowIndex + 1) % rowCount;
        mesh.addFace({ring[rowIndex], nextRing[rowIndex], nextRing[nextRow], ring[nextRow]});
    }
}

class Torus : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;

  private:
    TorusSettings readSettings() const;
};

TorusSettings Torus::readSettings() const
{
    TorusSettings settings;
    settings.columns = evalParmInt("columns");
    settings.rows = evalParmInt("rows");
    settings.ringRadius = evalParmFloat("radius", 0);
    settings.tubeRadius = evalParmFloat("radius", 1);
    return settings;
}

void Torus::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();
    const TorusSettings settings = readSettings();

    std::vector<std::vector<Offset>> rings;
    rings.reserve(settings.columns);
    for (int columnIndex = 0; columnIndex < settings.columns; ++columnIndex)
        rings.push_back(
            addTubeRing(*mesh, settings, static_cast<float>(columnIndex) / settings.columns)
        );

    for (int columnIndex = 0; columnIndex < settings.columns; ++columnIndex)
    {
        const int nextColumn = (columnIndex + 1) % settings.columns;
        addBandFaces(*mesh, rings[columnIndex], rings[nextColumn]);
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

ENZO_REGISTER_NODE(torus, Torus)
