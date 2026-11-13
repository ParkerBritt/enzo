#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Primitives/Primitive.h"
#include <memory>
#include <vector>

namespace {

using namespace enzo;

class CopyToPoints : public nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void CopyToPoints::cook()
{
    if (!outputRequested(0)) return;

    NodePacket prototypePacket = cloneInputPacket(0);
    NodePacket pointPacket = cloneInputPacket(1);
    NodePacket outputPacket;

    // Collects a transform for each target point, or a primitive's transform when it has no points.
    std::vector<Transform> transforms;
    for (const Transform& transform : pointPacket.getTransforms(TransformClass::POINT_PRIORITY))
        transforms.push_back(transform);

    for (const geo::PrimPtr& prim : prototypePacket.getPrimitives())
    {
        if (prim->transformType() == TransformClass::NONE) continue;

        // Fast copy for meshes
        if (const auto mesh = std::dynamic_pointer_cast<const geo::Mesh>(prim))
        {
            outputPacket.attemptMerge(mesh->copyToTransforms(transforms));
            continue;
        }

        // Slower copy for every other primitive type
        // It's fine that it's slower since they're usually much less expensive to copy
        for (const Transform& transform : transforms)
        {
            geo::PrimPtr copy = prim->clone();
            copy->applyTransform(transform, TransformClass::POINT_PRIORITY);
            outputPacket.attemptMerge(copy);
        }
    }

    setOutputPacket(0, std::move(outputPacket));
}

} // namespace

ENZO_REGISTER_NODE(copyToPoints, CopyToPoints)
