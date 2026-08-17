#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshShapes.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/Geometry>
#include <memory>
#include <numbers>
#include <vector>

namespace {

class Circle : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Circle::cook()
{
    using namespace enzo;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();

    int numPoints = evalParmInt("divisions");
    float uniform_scale = evalParmFloat("uniform_scale");
    std::string orientation = evalParmString("orientation");
    std::string arc_type = evalParmString("arc");
    float arcBegin = evalParmFloat("arc_angles", 0) / 360;
    float arcEnd = evalParmFloat("arc_angles", 1) / 360;
    enzo::Vector3 center = evalParmVector3("center");
    enzo::Vector3 rotate = evalParmVector3("rotate");
    enzo::Vector2 radius = evalParmVector2("radius");

    float arcDelta = arcEnd - arcBegin;
    if (arc_type == "closed") arcDelta = 1;

    const bool isArc = (arc_type != "closed");

    numPoints += isArc;

    std::vector<enzo::Vector3> newPointPositions;
    newPointPositions.reserve(numPoints);

    // Sliced and closed arc has one extra point in the middle for the triangles to anchor to
    if (arc_type == "sliced_arc" || arc_type == "closed_arc")
        newPointPositions.push_back({0, 0, 0});

    for (int divisionIndex = 0; divisionIndex < numPoints; ++divisionIndex)
    {
        float pointU = static_cast<float>(divisionIndex) / (numPoints - 1 * isArc);
        float angle = (arcBegin + pointU * arcDelta) * std::numbers::pi * 2;
        float u = sin(angle) * uniform_scale * radius[0];
        float v = cos(angle) * uniform_scale * radius[1];

        enzo::Vector3 position;
        if (orientation == "xy")
            position = {u, v, 0};
        else if (orientation == "yz")
            position = {0, u, v};
        else
            position = {u, 0, v};

        newPointPositions.push_back(position);
    }

    // Create points of circle
    auto newPoints = mesh->addPoints(newPointPositions);

    // Sliced arc gets one face for every two points, creating triangles
    if (arc_type == "sliced_arc")
    {
        for (size_t pointOffset = 1; pointOffset < newPoints.size() - 1; ++pointOffset)
        {
            mesh->addFace({newPoints[pointOffset], newPoints[pointOffset + 1], newPoints[0]});
        }
    }
    else
    {
        bool closed = arc_type != "open_arc";
        mesh->addFace(newPoints, closed);
    }

    mesh->applyTransform(enzo::Transform().translate(center).rotateEuler(rotate));

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(circle, Circle)
