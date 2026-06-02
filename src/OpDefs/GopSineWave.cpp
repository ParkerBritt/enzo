#include "OpDefs/GopSineWave.h"
#include "Engine/Operator/Mesh.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

GopSineWave::GopSineWave(enzo::nt::NetworkManager *network, enzo::op::OpInfo opInfo)
    : GeometryOpDef(network, opInfo) {}

void GopSineWave::cookOp(enzo::op::Context context) {
    using namespace enzo;

    if (outputRequested(0)) {
        NodePacket packet = context.cloneInputPacket(0);

        const floatT frequency = context.evalFloatParm("frequency");
        const floatT offset = context.evalFloatParm("offset");
        const bool radial = context.evalBoolParm("radial");

        for (size_t p = 0; p < packet.size(); ++p) {
            auto prim = packet.getPrimitive(p);
            if (prim->getType() != geo::PrimType::MESH)
                continue;
            auto geo = std::static_pointer_cast<geo::Mesh>(prim);
            const Offset pointCount = geo->getNumPoints();

            if (radial) {
                const Vector3 center(context.evalFloatParm("center", 0),
                                         context.evalFloatParm("center", 1),
                                         context.evalFloatParm("center", 2));
                tbb::parallel_for(
                    tbb::blocked_range<Offset>(0, pointCount),
                    [&geo, frequency, center, offset](tbb::blocked_range<Offset> range) {
                        for (Offset i = range.begin(); i != range.end(); ++i) {
                            Vector3 pos = geo->getPointPos(i);
                            pos +=
                                Vector3(0, sin((pos - center).norm() * frequency + offset), 0);
                            geo->setPointPos(i, pos);
                        }
                    });
            } else {
                tbb::parallel_for(tbb::blocked_range<Offset>(0, pointCount),
                                  [&geo, frequency, offset](tbb::blocked_range<Offset> range) {
                                      for (Offset i = range.begin(); i != range.end(); ++i) {
                                          Vector3 pos = geo->getPointPos(i);
                                          pos +=
                                              Vector3(0, sin(pos.x() * frequency + offset), 0);
                                          geo->setPointPos(i, pos);
                                      }
                                  });
            }
        }

        setOutputPacket(0, packet);
    }
}

std::vector<enzo::prm::Template> GopSineWave::parameterList()
{
    return {
        enzo::prm::Template(enzo::prm::Type::BOOL, enzo::prm::Name("radial", "Radial Mode")),
    enzo::prm::Template(enzo::prm::Type::XYZ, enzo::prm::Name("center", "Center"), 3),
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("frequency", "Frequency"),
                        enzo::prm::Default(1)),
    enzo::prm::Template(enzo::prm::Type::FLOAT, enzo::prm::Name("offset", "Offset"),
                        enzo::prm::Default(0))
    };
}
