#include "OpDefs/GopPath.h"
#include "Engine/Core/Types.h"

GopPath::GopPath(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType)
    : NodeDef(network, nodeType)
{
}

void GopPath::cook(enzo::nt::CookContext context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        String path = context.evalParmString("path");

        for (unsigned int p = 0; p < packet.size(); ++p)
        {
            packet.getPrimitive(p)->setPath(path);
        }

        setOutputPacket(0, packet);
    }
}

std::vector<enzo::prm::Template> GopPath::parameterList()
{
    return {enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("path", "Path"))};
}
