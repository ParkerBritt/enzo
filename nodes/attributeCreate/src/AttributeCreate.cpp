#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Selection/Selection.h"
#include <vector>

namespace {

/// @brief Returns the part of the geometry the attachTo name stands for.
enzo::attr::AttributeOwner getAttributeOwner(const enzo::String& attachTo)
{
    using namespace enzo;

    if (attachTo == "vertex") return attr::AttributeOwner::VERTEX;
    if (attachTo == "face") return attr::AttributeOwner::FACE;
    if (attachTo == "primitive") return attr::AttributeOwner::PRIMITIVE;
    return attr::AttributeOwner::POINT;
}

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

/// @brief Writes one value on every selected element of every selected primitive.
template <typename T>
void writeAttribute(
    enzo::NodePacket& packet,
    enzo::Selection& selection,
    enzo::attr::AttributeOwner owner,
    const enzo::String& name,
    enzo::attr::AttributeType type,
    const T& value
)
{
    using namespace enzo;

    for (geo::PrimPtr prim : selection.getPrims(packet))
    {
        const std::vector<Offset> offsets = getSelectedOffsets(selection, prim, owner);
        if (offsets.empty()) continue;

        attr::AttributeHandle<T> attribute(prim->addAttribute(owner, name, type));
        for (const Offset offset : offsets)
            attribute.setValue(offset, value);
    }
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

    const attr::AttributeOwner owner = getAttributeOwner(evalParmString("attachTo"));
    const String type = evalParmString("type");
    Selection selection(evalParmString("selection"));

    if (type == "int")
    {
        const intT value = evalParmInt("intValue");
        writeAttribute(packet, selection, owner, attributeName, attr::AttrType::intT, value);
    }
    else if (type == "vector")
    {
        const Vector3 value = evalParmVector3("vectorValue");
        writeAttribute(packet, selection, owner, attributeName, attr::AttrType::vectorT, value);
    }
    else if (type == "bool")
    {
        const boolT value = evalParmBool("boolValue");
        writeAttribute(packet, selection, owner, attributeName, attr::AttrType::boolT, value);
    }
    else
    {
        const floatT value = evalParmFloat("floatValue");
        writeAttribute(packet, selection, owner, attributeName, attr::AttrType::floatT, value);
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(attributeCreate, AttributeCreate)
