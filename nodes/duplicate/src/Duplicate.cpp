#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Primitive.h"
#include "Engine/Selection/Selection.h"
#include <vector>

namespace {

/// @brief Returns the scale a copy has reached after growing once per step.
enzo::Vector3 scaleAfterSteps(const enzo::Vector3& scalePerStep, enzo::intT steps)
{
    enzo::Vector3 scale = enzo::Vector3::Ones();
    for (enzo::intT step = 0; step < steps; ++step)
    {
        scale = scale.cwiseProduct(scalePerStep);
    }
    return scale;
}

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

    // Clones every copy before merging any of them, since merging grows the prim
    // the copies are taken from.
    std::vector<geo::PrimPtr> copies;
    Selection selection(selectionString);
    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        for (intT copyIndex = 1; copyIndex < count; ++copyIndex)
        {
            const floatT stepsFromOriginal = static_cast<floatT>(copyIndex);

            // Scale runs first, then the rotation, then the translation.
            const enzo::Transform transform =
                enzo::Transform()
                    .translate(translatePerCopy * stepsFromOriginal)
                    .rotateEuler(rotatePerCopy * stepsFromOriginal)
                    .scale(scaleAfterSteps(scalePerCopy, copyIndex));

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
