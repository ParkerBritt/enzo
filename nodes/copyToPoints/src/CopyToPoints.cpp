#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Primitive.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace {

class CopyToPoints : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void CopyToPoints::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    // TODO: don't need to clone the input packets, should really just get a const reference to
    // them Actually should implement a copy on write class for them, only copying modified
    // attributes I made an issue for it here https://github.com/ParkerBritt/enzo/issues/42
    NodePacket prototypePacket = cloneInputPacket(0);
    NodePacket pointPacket = cloneInputPacket(1);
    NodePacket outputPacket;

    // Iterate through every primitve on the point packet, getting their transforms
    // POINT_PRIORITY means it will iterate over points, using the P attribute if possible,
    // otherwise it will fall back to using primitive transforms (transform attribute) if the
    // primitive has any
    for (Transform transform : pointPacket.getTransforms(TransformClass::POINT_PRIORITY))
    {

        // Iterate through every prototype primitive, copying it onto the point
        for (auto& prim : prototypePacket.getPrimitives())
        {

            // Skip primitives that cannot be transformed
            if (prim->transformType() == TransformClass::NONE) continue;

            // Copy and transform primitive
            std::shared_ptr<geo::Primitive> newPrim = prim->clone();
            newPrim->applyTransform(transform, TransformClass::POINT_PRIORITY);

            // Add back to packet
            outputPacket.attemptMerge(newPrim);
        }
    }

    setOutputPacket(0, outputPacket);
    std::cout << "done\n";
}

} // namespace

ENZO_REGISTER_NODE(copyToPoints, CopyToPoints)
