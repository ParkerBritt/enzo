#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"

namespace {

class Path : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Path::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    String path = evalParmString("path");

    for (const geo::PrimPtr& prim : packet.getPrimitives())
        packet.movePrim(prim, path);

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(path, Path)
