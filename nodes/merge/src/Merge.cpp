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

    NodePacket output;

    // The output index each path sits at
    std::unordered_map<String, size_t> indexByPath;

    const unsigned int inputCount = getInputCount();
    for (unsigned int inputIndex = 0; inputIndex < inputCount; ++inputIndex)
    {
        NodePacket input = cloneInputPacket(inputIndex);
        for (size_t primIndex = 0; primIndex < input.size(); ++primIndex)
        {
            auto prim = input.getPrimitive(primIndex);
            const String path = prim->getPath();
            auto placed = indexByPath.find(path);
            if (placed == indexByPath.end())
            {
                indexByPath[path] = output.size();
                output.addPrimitive(prim);
                continue;
            }

            auto destination = output.getPrimitive(placed->second);
            const bool bothAreMeshes = destination->getType() == geo::PrimType::MESH
                                       && prim->getType() == geo::PrimType::MESH;
            if (bothAreMeshes)
                std::static_pointer_cast<geo::Mesh>(destination)
                    ->merge(*std::static_pointer_cast<geo::Mesh>(prim));
        }
    }

    setOutputPacket(0, output);
}

} // namespace

ENZO_REGISTER_NODE(merge, Merge)
