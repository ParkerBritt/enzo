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

    const Matrix4 transform = Transform()
                                  .translate(evalParmVector3("transform"))
                                  .rotateEuler(evalParmVector3("rotate"))
                                  .getMatrix();

    auto camera = std::make_shared<geo::Camera>();
    camera->setTransform(transform);

    NodePacket packet;
    packet.addPrimitive(std::move(camera));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(camera, Camera)
