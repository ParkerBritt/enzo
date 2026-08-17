
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Selection/Selection.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

namespace {

class Delete : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Delete::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const String selectionStr = evalParmString("selection", 0);
    const bool invert = evalParmBool("invert");
    const bool keepPoints = evalParmBool("keep_points");
    enzo::Selection selection(selectionStr);
    selection.setInverted(invert);

    for (auto prim : selection.getPrims(packet))
    {
        // Whole-prim deletion (skipped when keeping points; falls through to face/vertex
        // deletion)
        if (!keepPoints && selection.containsPrim(prim, true))
        {
            packet.removePrim(prim->getPath());
            continue;
        }

        // Delete points
        if (!keepPoints && prim->hasPoints())
        {
            prim->deletePoints(selection.getPoints(prim));
        }

        // Prim specific deletion
        switch (prim->getType())
        {
        case geo::PrimType::MESH:
        {
            auto meshPrim = std::dynamic_pointer_cast<geo::Mesh>(prim);
            meshPrim->deleteFaces(selection.getFaces(prim), !keepPoints);
            meshPrim->deleteVertices(selection.getVertices(prim));
            break;
        }
        default:
            break;
        }
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(delete, Delete)
