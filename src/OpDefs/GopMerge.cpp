#include "OpDefs/GopMerge.h"
#include "Engine/Types.h"
#include <unordered_map>
#include <string>

GopMerge::GopMerge(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
: GeometryOpDef(network, opInfo)
{

}

void GopMerge::cookOp(enzo::op::Context context)
{
    using namespace enzo;

    if(outputRequested(0))
    {
        NodePacket packet0 = context.cloneInputPacket(0);
        NodePacket packet1 = context.cloneInputPacket(1);

        // Index primitives from input 0 by path
        std::unordered_map<bt::String, size_t> pathIndex;
        for(size_t i = 0; i < packet0.size(); ++i)
        {
            pathIndex[packet0.getPrimitive(i).getPath()] = i;
        }

        // For each primitive in input 1, merge if path conflicts, otherwise append
        NodePacket output = std::move(packet0);
        for(size_t i = 0; i < packet1.size(); ++i)
        {
            auto& prim = packet1.getPrimitive(i);
            auto it = pathIndex.find(prim.getPath());
            if(it != pathIndex.end())
            {
                output.getPrimitive(it->second).merge(prim);
            }
            else
            {
                output.addPrimitive(prim);
            }
        }

        setOutputPacket(0, output);
    }

}

enzo::prm::Template GopMerge::parameterList[] =
{
    enzo::prm::Terminator
};
