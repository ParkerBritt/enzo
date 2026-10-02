#include "Engine/Expression/ScriptContext.h"
#include "Engine/Network/NodePacket.h"
#include "Engine/Primitives/Mesh.h"
#include <algorithm>

namespace enzo::expr {

ScriptContext::ScriptContext(nt::NodeId currentNode, const NodePacket& input)
    : ExpressionContext(currentNode)
{
    for (const std::shared_ptr<geo::Primitive>& primitive : input.getPrimitives())
    {
        if (primitive->getType() != geo::PrimType::MESH)
        {
            inputMeshes_.push_back(nullptr);
            attributeReaders_.push_back(std::nullopt);
            continue;
        }

        auto mesh = std::static_pointer_cast<const geo::Mesh>(primitive);
        attributeReaders_.emplace_back(*mesh);
        inputMeshes_.push_back(std::move(mesh));
    }
}

const geo::Mesh& ScriptContext::getInputMesh(size_t primitiveIndex) const
{
    return *inputMeshes_[primitiveIndex];
}

const MeshAttributeReader& ScriptContext::getAttributeReader(size_t primitiveIndex) const
{
    return *attributeReaders_[primitiveIndex];
}

void ScriptContext::addWarning(const String& warning) const
{
    std::lock_guard lock(warningMutex_);
    if (std::ranges::find(warnings_, warning) != warnings_.end()) return;
    warnings_.push_back(warning);
}

} // namespace enzo::expr
