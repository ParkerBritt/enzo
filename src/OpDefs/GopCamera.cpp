#include "OpDefs/GopCamera.h"
#include "Engine/Operator/Camera.h"
#include "Engine/Types.h"
#include <cmath>
#include "Engine/Parameter/Range.h"

GopCamera::GopCamera(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{
}

void GopCamera::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        NodePacket packet;
        auto cam = std::make_shared<geo::Camera>();

        bt::floatT tx = context.evalFloatParm("transform", 0);
        bt::floatT ty = context.evalFloatParm("transform", 1);
        bt::floatT tz = context.evalFloatParm("transform", 2);

        bt::floatT rx = context.evalFloatParm("rotate", 0) * M_PI / 180.0;
        bt::floatT ry = context.evalFloatParm("rotate", 1) * M_PI / 180.0;
        bt::floatT rz = context.evalFloatParm("rotate", 2) * M_PI / 180.0;

        // Rotation matrices (XYZ order)
        bt::Matrix4 rotX = bt::Matrix4::Identity();
        rotX(1,1) =  std::cos(rx); rotX(1,2) = -std::sin(rx);
        rotX(2,1) =  std::sin(rx); rotX(2,2) =  std::cos(rx);

        bt::Matrix4 rotY = bt::Matrix4::Identity();
        rotY(0,0) =  std::cos(ry); rotY(0,2) =  std::sin(ry);
        rotY(2,0) = -std::sin(ry); rotY(2,2) =  std::cos(ry);

        bt::Matrix4 rotZ = bt::Matrix4::Identity();
        rotZ(0,0) =  std::cos(rz); rotZ(0,1) = -std::sin(rz);
        rotZ(1,0) =  std::sin(rz); rotZ(1,1) =  std::cos(rz);

        bt::Matrix4 xform = bt::Matrix4::Identity();
        xform(0, 3) = tx;
        xform(1, 3) = ty;
        xform(2, 3) = tz;

        // T * Rz * Ry * Rx
        xform = xform * rotZ * rotY * rotX;
        cam->setTransform(xform);

        packet.addPrimitive(std::move(cam));
        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopCamera::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("transform", "Transform"), enzo::prm::Default(0), 3),
    enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("rotate", "Rotate"), enzo::prm::Default(0), 3, enzo::prm::Range(0, 360)),
    enzo::prm::Terminator
};
