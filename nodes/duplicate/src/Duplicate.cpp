#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Primitive.h"
#include "Engine/Selection/Selection.h"
#include <vector>

namespace {

class Duplicate : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Duplicate::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    NodePacket packet = cloneInputPacket(0);

    const String selectionString = evalParmString("selection");
    const intT count = evalParmInt("count");
    const Vector3 translatePerCopy = evalParmVector3("translate");
    const Vector3 rotatePerCopy = evalParmVector3("rotate");
    const Vector3 scalePerCopy = evalParmVector3("scale") * evalParmFloat("uniform_scale");
    const TransformOrder transformOrder = getTransformOrder(evalParmString("transform_order"));

    // Clones every copy before merging any of them, since merging grows the prim
    // the copies are taken from.
    std::vector<geo::PrimPtr> copies;
    Selection selection(selectionString);
    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        for (intT copyIndex = 1; copyIndex < count; ++copyIndex)
        {
            const floatT stepsFromOriginal = static_cast<floatT>(copyIndex);
            const Vector3 translation = translatePerCopy * stepsFromOriginal;
            const Vector3 rotation = rotatePerCopy * stepsFromOriginal;
            const Vector3 scale = scalePerCopy.array().pow(stepsFromOriginal).matrix();

            const enzo::Transform transform =
                enzo::Transform::fromComponents(translation, rotation, scale, transformOrder);

            geo::PrimPtr copy = prim->clone();
            copy->applyTransform(transform, TransformClass::POINT_PRIORITY);
            copies.push_back(copy);
        }
    }

    for (geo::PrimPtr copy : copies)
    {
        packet.attemptMerge(copy);
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(duplicate, Duplicate)
