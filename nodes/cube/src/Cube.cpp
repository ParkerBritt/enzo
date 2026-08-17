#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/MeshShapes.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <Eigen/Geometry>

namespace {

class Cube : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Cube::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    // Read shape parameters.
    const floatT sizeX = evalParmFloat("size", 0);
    const floatT sizeY = evalParmFloat("size", 1);
    const floatT sizeZ = evalParmFloat("size", 2);

    const floatT centerX = evalParmFloat("center", 0);
    const floatT centerY = evalParmFloat("center", 1);
    const floatT centerZ = evalParmFloat("center", 2);

    const floatT uniformScale = evalParmFloat("uniformScale");

    const floatT rotateX = evalParmFloat("rotate", 0) * M_PI / 180.0;
    const floatT rotateY = evalParmFloat("rotate", 1) * M_PI / 180.0;
    const floatT rotateZ = evalParmFloat("rotate", 2) * M_PI / 180.0;

    // Build an axis aligned cube around the origin so rotation pivots on its center.
    const Vector3 scaledSize(sizeX * uniformScale, sizeY * uniformScale, sizeZ * uniformScale);
    auto mesh = utils::buildCube(scaledSize, Vector3(0, 0, 0));

    // Compose rotation then translation into a single homogeneous transform.
    Eigen::Affine3f transform = Eigen::Affine3f::Identity();
    transform.translate(Vector3(centerX, centerY, centerZ));
    transform.rotate(Eigen::AngleAxisf(rotateX, Vector3::UnitX()));
    transform.rotate(Eigen::AngleAxisf(rotateY, Vector3::UnitY()));
    transform.rotate(Eigen::AngleAxisf(rotateZ, Vector3::UnitZ()));

    mesh->applyTransform(Matrix4(transform.matrix()));

    NodePacket packet;
    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(cube, Cube)
