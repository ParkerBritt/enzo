#include "Engine/Script/MeshAttributeReader.h"
#include "Engine/Primitives/Mesh.h"

namespace enzo::script {

namespace {
size_t getOwnerSlot(attr::AttributeOwner owner) { return static_cast<size_t>(owner); }

Offset getElementCount(const geo::Mesh& mesh, attr::AttributeOwner owner)
{
    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return mesh.getNumPoints();
    case attr::AttributeOwner::VERTEX:
        return mesh.getNumVerts();
    case attr::AttributeOwner::FACE:
        return mesh.getNumFaces();
    default:
        return 1;
    }
}

ScriptAttributeHandle makeHandle(const std::shared_ptr<const attr::Attribute>& attribute)
{
    switch (attribute->getType())
    {
    case attr::AttributeType::floatT:
        return attr::AttributeHandleRO<floatT>(attribute);
    case attr::AttributeType::intT:
        return attr::AttributeHandleRO<intT>(attribute);
    case attr::AttributeType::vectorT:
        return attr::AttributeHandleRO<Vector3>(attribute);
    case attr::AttributeType::boolT:
        return attr::AttributeHandleRO<boolT>(attribute);
    default:
        return std::monostate{};
    }
}
} // namespace

MeshAttributeReader::MeshAttributeReader(const geo::Mesh& mesh)
{
    for (const attr::AttributeOwner owner : attr::getAllOwners())
    {
        elementCounts_[getOwnerSlot(owner)] = getElementCount(mesh, owner);

        std::vector<ScriptAttribute>& attributes = attributes_[getOwnerSlot(owner)];
        for (const auto& attribute : mesh.getAttributes(owner, true))
        {
            attributes.push_back(
                ScriptAttribute{attribute->getName(), attribute->getType(), makeHandle(attribute)}
            );
        }
    }
}

bool MeshAttributeReader::hasElement(attr::AttributeOwner owner, intT index) const
{
    return index >= 0 && index < static_cast<intT>(elementCounts_[getOwnerSlot(owner)]);
}

const ScriptAttribute*
MeshAttributeReader::getAttribute(attr::AttributeOwner owner, std::string_view name) const
{
    for (const ScriptAttribute& attribute : attributes_[getOwnerSlot(owner)])
    {
        if (attribute.name == name) return &attribute;
    }
    return nullptr;
}

} // namespace enzo::script
