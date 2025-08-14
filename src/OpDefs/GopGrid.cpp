#include "OpDefs/GopGrid.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <cmath>
#include <cstdio>
#include <tbb/parallel_for.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

GopGrid::GopGrid(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopGrid::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        geo::Geometry geo;
        bt::floatT width = context.evalFloatParm("size", 0);
        bt::floatT height = context.evalFloatParm("size", 1);

        const bt::intT columns = context.evalIntParm("columns");
        const bt::intT rows = context.evalIntParm("rows");
        if(columns<=0 || rows<=0)
        {
            setOutputGeometry(0, geo);
            return;
        }
        
        const bt::floatT centerOffsetX = width/2.0;
        const bt::floatT centerOffsetY = height/2.0;

        const bt::floatT columnDivisor = std::max<bt::floatT>(columns-1, 1);
        const bt::floatT rowDivisor = std::max<bt::floatT>(rows-1, 1);
        // add points
        for(int i=0;i<columns;i++)
        {
            for(int j=0;j<rows;++j)
            {
                const bt::floatT x = i/columnDivisor*width-centerOffsetX;
                const bt::floatT z = j/rowDivisor*height-centerOffsetY;
                geo.addPoint(bt::Vector3(x, 0, z));
            }
        }

        if(columns > 1 && rows > 1)
        {
            // add faces
            for(int i=0;i<std::floor((columns-1)*(rows)-1);i++)
            {
                const int endOffset = (i+1)%rows==0;
                const ga::Offset startPt = i+endOffset; 
                geo.addFace({startPt,startPt+rows,startPt+rows+1,startPt+1});
            }
        }
        else
        {
            // add lines
            const size_t iterationLimit = std::max(columns, rows)-1; 
            for(int i=0;i<iterationLimit;i++)
            {
                const ga::Offset startPt = i; 
                geo.addFace({startPt,startPt+1}, false);
            }
        }

        setOutputGeometry(0, geo);
    }

}

enzo::prm::Template GopGrid::parameterList[] =
{
    enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("size", "Size"), enzo::prm::Default(10), 2, enzo::prm::Range(0, enzo::prm::RangeFlag::UNLOCKED, 100, enzo::prm::RangeFlag::UNLOCKED)),
    enzo::prm::Template(
        enzo::prm::Type::INT,
        enzo::prm::Name("rows", "Rows"),
        enzo::prm::Default(10),
        1,
        enzo::prm::Range(1, enzo::prm::RangeFlag::LOCKED, 100, enzo::prm::RangeFlag::UNLOCKED)
    ),
    enzo::prm::Template(
        enzo::prm::Type::INT,
        enzo::prm::Name("columns", "Columns"),
        enzo::prm::Default(10),
        1,
        enzo::prm::Range(1, enzo::prm::RangeFlag::LOCKED, 100, enzo::prm::RangeFlag::UNLOCKED)
    ),
    enzo::prm::Terminator
};
