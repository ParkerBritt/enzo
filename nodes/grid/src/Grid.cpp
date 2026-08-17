#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <tbb/parallel_for.h>

namespace {

class Grid : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Grid::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet;
    auto geo = std::make_shared<geo::Mesh>();
    floatT width = evalParmFloat("size", 0);
    floatT height = evalParmFloat("size", 1);

    const intT columns = evalParmInt("columns");
    const intT rows = evalParmInt("rows");
    if (columns <= 0 || rows <= 0)
    {
        packet.addPrimitive(std::move(geo));
        setOutputPacket(0, packet);
        return;
    }

    const floatT centerOffsetX = width / 2.0;
    const floatT centerOffsetY = height / 2.0;

    const floatT columnDivisor = std::max<floatT>(columns - 1, 1);
    const floatT rowDivisor = std::max<floatT>(rows - 1, 1);
    // add points
    for (int i = 0; i < columns; i++)
    {
        for (int j = 0; j < rows; ++j)
        {
            const floatT x = i / columnDivisor * width - centerOffsetX;
            const floatT z = j / rowDivisor * height - centerOffsetY;
            geo->addPoint(Vector3(x, 0, z));
        }
    }

    if (columns > 1 && rows > 1)
    {
        // add faces
        for (int col = 0; col < columns - 1; ++col)
        {
            for (int row = 0; row < rows - 1; ++row)
            {
                const Offset startPt = col * rows + row;
                geo->addFace({startPt, startPt + 1, startPt + rows + 1, startPt + rows});
            }
        }
    }
    else
    {
        // add lines
        const size_t iterationLimit = std::max(columns, rows) - 1;
        for (int i = 0; i < iterationLimit; i++)
        {
            const Offset startPt = i;
            geo->addFace({startPt, startPt + 1}, false);
        }
    }

    packet.addPrimitive(std::move(geo));
    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(grid, Grid)
