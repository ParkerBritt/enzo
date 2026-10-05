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

// Returns the rotation in degrees that winds the spiral around the named axis.
// The spiral is built around Y, so Y itself needs no rotation.
enzo::Vector3 getAxisRotation(const std::string& axis)
{
    if (axis == "x") return {0, 0, -90};
    if (axis == "z") return {90, 0, 0};
    return {0, 0, 0};
}

class Spiral : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Spiral::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();

    const float startRadius = evalParmFloat("radius", 0);
    const float endRadius = evalParmFloat("radius", 1);
    const float height = evalParmFloat("height");
    const float turns = evalParmFloat("turns");
    const float turnSign = evalParmString("turnDirection") == "clockwise" ? -1 : 1;
    const int pointsPerTurn = evalParmInt("pointsPerTurn");
    const int segmentCount = std::max(1, static_cast<int>(std::lround(turns * pointsPerTurn)));

    std::vector<Vector3> positions;
    positions.reserve(segmentCount + 1);
    for (int pointIndex = 0; pointIndex <= segmentCount; ++pointIndex)
    {
        const float lengthFraction = static_cast<float>(pointIndex) / segmentCount;
        const float angle = lengthFraction * turns * turnSign * std::numbers::pi_v<float> * 2;
        const float radius = std::lerp(startRadius, endRadius, lengthFraction);
        positions.push_back(
            {std::sin(angle) * radius, (lengthFraction - 0.5f) * height, std::cos(angle) * radius}
        );
    }

    constexpr bool isClosed = false;
    mesh->addFace(mesh->addPoints(positions), isClosed);

    mesh->applyTransform(Transform()
                             .translate(evalParmVector3("center"))
                             .rotateEuler(evalParmVector3("rotate"))
                             .rotateEuler(getAxisRotation(evalParmString("axis")))
                             .scale(evalParmFloat("uniformScale")));

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(spiral, Spiral)
