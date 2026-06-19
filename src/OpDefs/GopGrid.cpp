#include "OpDefs/GopGrid.h"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <tbb/parallel_for.h>

GopGrid::GopGrid(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo)
{
}

void GopGrid::cookOp(enzo::op::CookContext context)
{
    using namespace enzo;

    if (outputRequested(0))
    {
        NodePacket packet;
        auto geo = std::make_shared<geo::Mesh>();
        floatT width = context.evalParmFloat("size", 0);
        floatT height = context.evalParmFloat("size", 1);

        const intT columns = context.evalParmInt("columns");
        const intT rows = context.evalParmInt("rows");
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
}

std::vector<enzo::prm::Template> GopGrid::parameterList()
{
    return {
        enzo::prm::Template(
            enzo::prm::Type::XYZ,
            enzo::prm::Name("size", "Size"),
            enzo::prm::Default(10),
            2,
            enzo::prm::Range(0, 100)
        ),
        enzo::prm::Template(
            enzo::prm::Type::INT,
            enzo::prm::Name("rows", "Rows"),
            enzo::prm::Default(10),
            1,
            enzo::prm::Range(1, 100, enzo::prm::RangeFlag::LOCKED, enzo::prm::RangeFlag::UNLOCKED)
        ),
        enzo::prm::Template(
            enzo::prm::Type::INT,
            enzo::prm::Name("columns", "Columns"),
            enzo::prm::Default(10),
            1,
            enzo::prm::Range(1, 100, enzo::prm::RangeFlag::LOCKED, enzo::prm::RangeFlag::UNLOCKED)
        )
    };
}
