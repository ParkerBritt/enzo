#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Selection/Selection.h"
#include <optional>
#include <vector>

namespace {

/// @brief Returns the offsets of the elements the selection covers on one primitive.
///
/// @note A primitive owner holds a single value, so it is either covered whole or
///       not at all.
std::vector<enzo::Offset> getSelectedOffsets(
    enzo::Selection& selection,
    enzo::geo::PrimPtr prim,
    enzo::attr::AttributeOwner owner
)
{
    using namespace enzo;

    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return selection.getPoints(prim);
    case attr::AttributeOwner::VERTEX:
        return selection.getVertices(prim);
    case attr::AttributeOwner::FACE:
        return selection.getFaces(prim);
    case attr::AttributeOwner::PRIMITIVE:
        return {0};
    }
    return {};
}

/**
 * @brief Writes one value on every selected element of every selected primitive.
 *
 * @return False when the name is taken by an internal attribute or by an intrinsic of
 *         another type.
 */
template <typename T>
bool writeAttribute(
    enzo::NodePacket& packet,
    enzo::Selection& selection,
    enzo::attr::AttributeOwner owner,
    const enzo::String& name,
    const T& value
)
{
    using namespace enzo;

    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        const std::vector<Offset> offsets = getSelectedOffsets(selection, prim, owner);
        if (offsets.empty()) continue;

        std::optional<attr::AttributeHandle<T>> attribute = prim->tryAddAttribute<T>(owner, name);
        if (!attribute) return false;
        for (const Offset offset : offsets)
            attribute->setValue(offset, value);
    }
    return true;
}

class AttributeCreate : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void AttributeCreate::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const String attributeName = evalParmString("name");
    if (attributeName.empty())
    {
        throwError("The attribute needs a name.");
        return;
    }

    NodePacket packet = cloneInputPacket(0);

    const std::optional<attr::AttributeOwner> owner = attr::getOwner(evalParmString("attachTo"));
    if (!owner)
    {
        throwError("Unknown attach point.");
        return;
    }

    const String type = evalParmString("type");
    Selection selection(evalParmString("selection"));

    bool written = false;
    if (type == "int")
    {
        const intT value = evalParmInt("intValue");
        written = writeAttribute(packet, selection, *owner, attributeName, value);
    }
    else if (type == "vector")
    {
        const Vector3 value = evalParmVector3("vectorValue");
        written = writeAttribute(packet, selection, *owner, attributeName, value);
    }
    else if (type == "bool")
    {
        const boolT value = evalParmBool("boolValue");
        written = writeAttribute(packet, selection, *owner, attributeName, value);
    }
    else if (type == "float")
    {
        const floatT value = evalParmFloat("floatValue");
        written = writeAttribute(packet, selection, *owner, attributeName, value);
    }
    else
    {
        throwError("Unknown attribute type.");
        return;
    }

    if (!written)
    {
        throwError("The attribute " + attributeName + " can't hold " + type + " values.");
        return;
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(attributeCreate, AttributeCreate)
