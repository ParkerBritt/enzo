#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <memory>
#include <vector>

namespace {

class Line : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Line::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    std::shared_ptr<geo::Mesh> mesh = std::make_shared<geo::Mesh>();

    const Vector3 origin = evalParmVector3("origin");
    const Vector3 direction = evalParmVector3("direction").normalized();
    const float length = evalParmFloat("length");
    const int pointCount = evalParmInt("points");

    std::vector<Vector3> positions;
    positions.reserve(pointCount);
    for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
    {
        const float lengthFraction = static_cast<float>(pointIndex) / (pointCount - 1);
        positions.push_back(origin + direction * length * lengthFraction);
    }

    constexpr bool isClosed = false;
    mesh->addFace(mesh->addPoints(positions), isClosed);

    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(line, Line)
