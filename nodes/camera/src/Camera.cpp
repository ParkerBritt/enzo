#include "Engine/Primitives/Camera.h"
#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Range.h"

namespace {

class Camera : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Camera::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const Vector3 translation(
        evalParmFloat("transform", 0),
        evalParmFloat("transform", 1),
        evalParmFloat("transform", 2)
    );
    const Vector3 rotation(
        evalParmFloat("rotate", 0),
        evalParmFloat("rotate", 1),
        evalParmFloat("rotate", 2)
    );

    auto camera = std::make_shared<geo::Camera>();
    camera->setTransform(Transform().translate(translation).rotateEuler(rotation).getMatrix());

    NodePacket packet;
    packet.addPrimitive(std::move(camera));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(camera, Camera)
