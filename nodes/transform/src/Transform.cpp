#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Selection/Selection.h"
#include <memory>
#include <vector>

namespace {

/// @brief Moves the given points of a mesh, leaving the rest of the mesh where it is.
void transformPoints(
    enzo::geo::PrimPtr prim,
    const std::vector<enzo::Offset>& pointOffsets,
    const enzo::Transform& transform
)
{
    auto mesh = std::dynamic_pointer_cast<enzo::geo::Mesh>(prim);
    if (!mesh) return;

    for (const enzo::Offset pointOffset : pointOffsets)
    {
        mesh->setPointPos(pointOffset, transform * mesh->getPointPos(pointOffset));
    }
}

class Transform : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Transform::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const String selectionString = evalParmString("selection");
    const Vector3 translate = evalParmVector3("translate");
    const Vector3 rotate = evalParmVector3("rotate");
    const Vector3 scale = evalParmVector3("scale");
    const floatT uniformScale = evalParmFloat("uniform_scale");
    const Vector3 pivot = evalParmVector3("pivot");
    const Vector3 pivotRotate = evalParmVector3("pivot_rotate");
    const TransformOrder order = getTransformOrder(evalParmString("transform_order"));

    // Build the pivot transform
    const enzo::Transform pivotTransform =
        enzo::Transform().translate(pivot).rotateEuler(pivotRotate);

    // Build the local transform
    const enzo::Transform pivotRotation = enzo::Transform().rotateEuler(pivotRotate);
    const Vector3 pivotTranslate = pivotRotation.getInverse() * translate;
    const enzo::Transform localTransform =
        enzo::Transform::fromComponents(pivotTranslate, rotate, scale * uniformScale, order);

    // Combine transforms
    // Moves the pivot to the origin, applies the local transform, then moves the pivot back.
    const enzo::Transform transform = pivotTransform * localTransform * pivotTransform.getInverse();

    Selection selection(selectionString);
    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        const bool wholePrim = selection.containsPrim(prim, true);
        if (wholePrim)
        {
            prim->applyTransform(transform, TransformClass::POINT);
            continue;
        }
        transformPoints(prim, selection.getPoints(prim), transform);
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(transform, Transform)
