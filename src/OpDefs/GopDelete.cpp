
#include "OpDefs/GopDelete.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Selection.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

GopDelete::GopDelete(enzo::nt::NetworkManager *network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo) {}

void GopDelete::cookOp(enzo::op::Context context) {
    using namespace enzo;

    if (outputRequested(0)) {
        NodePacket packet = context.cloneInputPacket(0);

        const bt::String selectionStr = context.evalStringParm("selection", 0);
        enzo::Selection selection(selectionStr);

        for (auto prim : selection.getPrims(packet)) {
            // Whole-prim deletion
            if (selection.containsPrim(prim, true)) {
                packet.removePrim(prim->getPath());
                continue;
            }

            // Delete points
            // if (prim->hasPoints())
            // {
            //     prim->deletePoints(selection.getPoints(prim));
            // }

            // Prim specific deletion
            switch (prim->getType()) {
            case geo::PrimType::MESH: {
                auto meshPrim = std::dynamic_pointer_cast<geo::Mesh>(prim);
                meshPrim->deleteFaces(selection.getFaces(prim));
                // meshPrim->deleteVertices(selection.getVertices(prim));
                break;
            }
            default:
                break;
            }
        }

        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopDelete::parameterList[] = {
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("selection", "Selection")),
    enzo::prm::Terminator};
