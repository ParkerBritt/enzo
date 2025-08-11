#include "OpDefs/GopSineWave.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <cmath>
#include <cstdio>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

GopSineWave::GopSineWave(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopSineWave::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        geo::Geometry geo = context.cloneInputGeo(0);
        
        const ga::Offset pointCount = geo.getNumPoints();
        const bt::floatT frequency = context.evalFloatParm("frequency");
        tbb::parallel_for(tbb::blocked_range<ga::Offset>(0, pointCount), [&geo, frequency](tbb::blocked_range<ga::Offset> range){
            for(ga::Offset i=range.begin(); i!=range.end(); ++i)
            {
                bt::Vector3 pos = geo.getPointPos(i);
                pos += bt::Vector3(0, sin(pos.x()*frequency), 0);
                geo.setPointPos(i, pos);
            }
        });

        setOutputGeometry(0, geo);
    }

}

enzo::prm::Template GopSineWave::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("frequency", "Frequency"), enzo::prm::Default(1), 1),
    enzo::prm::Terminator
};
