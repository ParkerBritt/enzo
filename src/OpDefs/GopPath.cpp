#include "OpDefs/GopPath.h"
#include "Engine/Types.h"

GopPath::GopPath(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{
}

void GopPath::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        bt::String path = context.evalStringParm("path");

        for(unsigned int p = 0; p < packet.size(); ++p)
        {
            packet.getPrimitive(p)->setPath(path);
        }

        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopPath::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("path", "Path")),
    enzo::prm::Terminator
};
