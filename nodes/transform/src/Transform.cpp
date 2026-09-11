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

    const enzo::Transform transform =
        enzo::Transform::fromComponents(translate, rotate, scale * uniformScale);

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
