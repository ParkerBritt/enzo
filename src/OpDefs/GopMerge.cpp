#include "OpDefs/GopMerge.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <cmath>
#include <cstdio>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

GopMerge::GopMerge(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopMerge::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        // TODO: convert to NodePacket
        NodePacket packet = context.cloneInputPacket(0);
        setOutputPacket(0, packet);
    }

}

enzo::prm::Template GopMerge::parameterList[] =
{
    enzo::prm::Terminator
};
