#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Geometry/AngleAxis.h>
#include <Eigen/src/Geometry/Transform.h>
#include <numbers>

namespace {

class Transform : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Transform::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const floatT degreesToRadians = std::numbers::pi / 180.0;
    const floatT rotateX = evalParmFloat("rotate", 0) * degreesToRadians;
    const floatT rotateY = evalParmFloat("rotate", 1) * degreesToRadians;
    const floatT rotateZ = evalParmFloat("rotate", 2) * degreesToRadians;

    // Compose translation then rotation into one homogeneous transform.
    Eigen::Affine3f transform = Eigen::Affine3f::Identity();
    transform.translate(Vector3(
        evalParmFloat("translate", 0),
        evalParmFloat("translate", 1),
        evalParmFloat("translate", 2)
    ));
    transform.rotate(Eigen::AngleAxisf(rotateX, Vector3(1, 0, 0)));
    transform.rotate(Eigen::AngleAxisf(rotateY, Vector3(0, 1, 0)));
    transform.rotate(Eigen::AngleAxisf(rotateZ, Vector3(0, 0, 1)));

    const Matrix4 matrix = transform.matrix();
    for (unsigned int p = 0; p < packet.size(); ++p)
    {
        packet.getPrimitive(p)->applyTransform(matrix, TransformClass::POINT);
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(transform, Transform)
