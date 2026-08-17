#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Ramp.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Primitives/Mesh.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

namespace {

class SineWave : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void SineWave::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const floatT frequency = evalParmFloat("frequency");
    const floatT offset = evalParmFloat("offset");
    const bool radial = evalParmBool("radial");
    const prm::Ramp amplitude = evalParmRamp("amplitude");

    for (size_t p = 0; p < packet.size(); ++p)
    {
        auto prim = packet.getPrimitive(p);
        if (prim->getType() != geo::PrimType::MESH) continue;
        auto geo = std::static_pointer_cast<geo::Mesh>(prim);
        const Offset pointCount = geo->getNumPoints();

        if (radial)
        {
            const Vector3 center(
                evalParmFloat("center", 0),
                evalParmFloat("center", 1),
                evalParmFloat("center", 2)
            );
            tbb::parallel_for(
                tbb::blocked_range<Offset>(0, pointCount),
                [&geo, &amplitude, frequency, center, offset](
                    tbb::blocked_range<Offset> range
                ) {
                    for (Offset i = range.begin(); i != range.end(); ++i)
                    {
                        Vector3 pos = geo->getPointPos(i);
                        // The ramp remaps the wave from its zero to one domain.
                        const floatT wave = sin((pos - center).norm() * frequency + offset);
                        pos += Vector3(0, amplitude.sample((wave + 1) * 0.5f), 0);
                        geo->setPointPos(i, pos);
                    }
                }
            );
        }
        else
        {
            tbb::parallel_for(
                tbb::blocked_range<Offset>(0, pointCount),
                [&geo, &amplitude, frequency, offset](tbb::blocked_range<Offset> range) {
                    for (Offset i = range.begin(); i != range.end(); ++i)
                    {
                        Vector3 pos = geo->getPointPos(i);
                        // The ramp remaps the wave from its zero to one domain.
                        const floatT wave = sin(pos.x() * frequency + offset);
                        pos += Vector3(0, amplitude.sample((wave + 1) * 0.5f), 0);
                        geo->setPointPos(i, pos);
                    }
                }
            );
        }
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(sineWave, SineWave)
