#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"

namespace {

class Merge : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Merge::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket output;
    const unsigned int inputCount = getInputCount();
    for (unsigned int inputIndex = 0; inputIndex < inputCount; ++inputIndex)
    {
        NodePacket input = cloneInputPacket(inputIndex);
        for (const geo::PrimPtr& prim : input.getPrimitives())
            output.attemptMerge(prim);
    }

    setOutputPacket(0, output);
}

} // namespace

ENZO_REGISTER_NODE(merge, Merge)
