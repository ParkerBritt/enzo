
#include "OpDefs/GopDelete.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <cmath>
#include <cstdio>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

GopDelete::GopDelete(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopDelete::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        NodePacket packet = context.cloneInputPacket(0);

        const bt::String selectionStr = context.evalStringParm("Selection", 0);
        Selection selection = Selection::compose(packet, selectionStr);
        //
        // for(auto prim : selection.getPrimitives())
        // {
        //     // Check for whole prim deletion
        //     if(selection->containsFullPrim(prim))
        //     {
        //         packet->removePrim(prim);
        //         continue;
        //     }
        //
        //     // Delete points
        //     if prim->hasPoints()
        //     {
        //         meshPrim->deletePoints(selection->getPoints(meshPrim))
        //     }
        //
        //     // Prim specific deletion
        //     switch(prim->getType())
        //     {
        //         case geo::PrimType::MESH:
        //             auto meshPrim = dynamic_cast<Mesh>(prim)
        //             // Delete faces
        //             meshPrim->deleteFaces(selection->getFaces(meshPrim))
        //             // Delete verts
        //             meshPrim->deleteVertices(selection->getVertices(meshPrim))
        //     }
        //
        // }
        //

        setOutputPacket(0, packet);
    }

}

enzo::prm::Template GopDelete::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("group", "Group")),
    enzo::prm::Terminator
};
