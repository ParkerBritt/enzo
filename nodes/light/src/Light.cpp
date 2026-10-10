#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/PointLight.h"

namespace {

class Light : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Light::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const Matrix4 transform = Transform()
                                  .translate(evalParmVector3("translate"))
                                  .rotateEuler(evalParmVector3("rotate"))
                                  .getMatrix();

    auto light = std::make_shared<geo::PointLight>();
    light->setTransform(transform);
    light->setColor(evalParmVector3("color"));
    light->setIntensity(evalParmFloat("intensity"));

    NodePacket packet;
    packet.addPrimitive(std::move(light));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(light, Light)
