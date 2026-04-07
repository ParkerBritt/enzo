#include "OpDefs/GopCamera.h"
#include "Engine/Operator/Camera.h"
#include "Engine/Types.h"

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
        cam->setPosition(bt::Vector3(tx, ty, tz));

        packet.addPrimitive(std::move(cam));
        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopCamera::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("transform", "Transform"), enzo::prm::Default(0), 3),
    enzo::prm::Terminator
};
