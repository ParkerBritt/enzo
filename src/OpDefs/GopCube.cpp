#include "OpDefs/GopCube.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include "Engine/Utils/MeshShapes.h"
#include <Eigen/Geometry>

GopCube::GopCube(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{
}

void GopCube::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    // Read shape parameters.
    const bt::floatT sizeX = context.evalFloatParm("size", 0);
    const bt::floatT sizeY = context.evalFloatParm("size", 1);
    const bt::floatT sizeZ = context.evalFloatParm("size", 2);

    const bt::floatT centerX = context.evalFloatParm("center", 0);
    const bt::floatT centerY = context.evalFloatParm("center", 1);
    const bt::floatT centerZ = context.evalFloatParm("center", 2);

    const bt::floatT uniformScale = context.evalFloatParm("uniformScale");

    const bt::floatT rotateX = context.evalFloatParm("rotate", 0) * M_PI / 180.0;
    const bt::floatT rotateY = context.evalFloatParm("rotate", 1) * M_PI / 180.0;
    const bt::floatT rotateZ = context.evalFloatParm("rotate", 2) * M_PI / 180.0;

    // Build an axis aligned cube around the origin so rotation pivots on its center.
    const bt::Vector3 scaledSize(sizeX * uniformScale,
                                  sizeY * uniformScale,
                                  sizeZ * uniformScale);
    auto mesh = utils::buildCube(scaledSize, bt::Vector3(0, 0, 0));

    // Compose rotation then translation into a single homogeneous transform.
    Eigen::Affine3d transform = Eigen::Affine3d::Identity();
    transform.translate(Eigen::Vector3d(centerX, centerY, centerZ));
    transform.rotate(Eigen::AngleAxisd(rotateX, Eigen::Vector3d::UnitX()));
    transform.rotate(Eigen::AngleAxisd(rotateY, Eigen::Vector3d::UnitY()));
    transform.rotate(Eigen::AngleAxisd(rotateZ, Eigen::Vector3d::UnitZ()));

    mesh->applyTransform(bt::Matrix4(transform.matrix()));

    NodePacket packet;
    packet.addPrimitive(std::move(mesh));
    setOutputPacket(0, packet);
}

std::vector<enzo::prm::Template> GopCube::parameterList()
{
    using namespace enzo::prm;
    return {
        Template(Type::XYZ, Name("size", "Size"), Default(1), 3, Range(0, 100)),
        Template(Type::XYZ, Name("center", "Center"), Default(0), 3),
        Template(Type::XYZ, Name("rotate", "Rotation"), Default(0), 3),
        Template(Type::FLOAT, Name("uniformScale", "Uniform Scale"), Default(1), 1, Range(0, 10)),
    };
}
