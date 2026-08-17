#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <string>
#include <unordered_map>

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

    NodePacket packet0 = cloneInputPacket(0);
    NodePacket packet1 = cloneInputPacket(1);

    // Index primitives from input 0 by path
    std::unordered_map<String, size_t> pathIndex;
    for (size_t i = 0; i < packet0.size(); ++i)
    {
        pathIndex[packet0.getPrimitive(i)->getPath()] = i;
    }

    // For each primitive in input 1, merge if path conflicts, otherwise append
    NodePacket output = std::move(packet0);
    for (size_t i = 0; i < packet1.size(); ++i)
    {
        auto prim = packet1.getPrimitive(i);
        auto it = pathIndex.find(prim->getPath());
        if (it != pathIndex.end())
        {
            auto dst = output.getPrimitive(it->second);
            if (dst->getType() == geo::PrimType::MESH && prim->getType() == geo::PrimType::MESH)
            {
                std::static_pointer_cast<geo::Mesh>(dst)->merge(
                    *std::static_pointer_cast<geo::Mesh>(prim)
                );
            }
        }
        else
        {
            output.addPrimitive(prim);
        }
    }

    setOutputPacket(0, output);
}

} // namespace

ENZO_REGISTER_NODE(merge, Merge)
