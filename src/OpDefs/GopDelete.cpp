
#include "OpDefs/GopDelete.h"
#include "Engine/Core/Types.h"
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

GopDelete::GopDelete(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopDelete::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        const String selectionStr = context.evalParmString("selection", 0);
        const bool invert = context.evalParmBool("invert");
        const bool keepPoints = context.evalParmBool("keep_points");
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
}

std::vector<enzo::prm::Template> GopDelete::parameterList()
{
    return {
        enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("selection", "Selection")),
        enzo::prm::Template(enzo::prm::Type::BOOL, enzo::prm::Name("invert", "Invert Selection")),
        enzo::prm::Template(enzo::prm::Type::BOOL, enzo::prm::Name("keep_points", "Keep Points"))
    };
}
