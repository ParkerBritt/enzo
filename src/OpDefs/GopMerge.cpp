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
        geo::Geometry geo = context.cloneInputGeo(0);
        if(context.hasInput(1))
        {
            geo::Geometry geoSrc = context.cloneInputGeo(1);
            geo.merge(geoSrc);
        }
        setOutputGeometry(0, geo);
    }

}

enzo::prm::Template GopMerge::parameterList[] =
{
    enzo::prm::Terminator
};
