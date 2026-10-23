#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Selection/Selection.h"
#include <memory>

namespace {

class Reverse : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Reverse::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    Selection selection(evalParmString("selection"));
    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
        if (!mesh) continue;

        mesh->reverseFaces(selection.getFaces(prim));
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(reverse, Reverse)
