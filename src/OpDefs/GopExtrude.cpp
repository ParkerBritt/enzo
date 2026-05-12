#include "OpDefs/GopExtrude.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Operator/Selection.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <vector>
#include <icecream.hpp>

void extrude(enzo::geo::PrimPtr prim, std::vector<enzo::ga::Offset> faces)
{
    std::cout << "here2\n";
    std::shared_ptr<enzo::geo::Mesh> mesh = std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
    if(!mesh)
    {
        return;
    }

    for(auto face: faces)
    {
        auto facePoints = mesh->getFacePoints(face);
        
        for(auto pointOffset : facePoints)
        {
            auto pointPos = mesh->getPointPos(pointOffset);
            pointPos.y() += 1;
            mesh->addPoint(pointPos);
        }
    }

}

GopExtrude::GopExtrude(enzo::nt::NetworkManager *network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo) {}

void GopExtrude::cookOp(enzo::op::Context context) {
    using namespace enzo;

    if (outputRequested(0)) {
        NodePacket packet = context.cloneInputPacket(0);

        const bt::String selectionStr = context.evalStringParm("selection", 0);
        enzo::Selection selection(selectionStr);

        for (geo::PrimPtr prim : selection.getPrims(packet)) {
            extrude(prim, selection.getFaces(prim));
        }

        setOutputPacket(0, packet);
    }
}

enzo::prm::Template GopExtrude::parameterList[] = {
    enzo::prm::Template(enzo::prm::Type::STRING, enzo::prm::Name("selection", "Selection")),
    enzo::prm::Terminator};
