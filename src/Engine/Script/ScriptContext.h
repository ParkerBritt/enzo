#pragma once
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Script/MeshAttributeReader.h"
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace enzo {
class NodePacket;
}

namespace enzo::geo {
class Mesh;
}

namespace enzo::script {

/**
 * @brief The world a script reads during one cook of a node.
 *
 * Adds the meshes of the node's input to what an expression reads, each looked
 * up by its primitive index in the input.
 *
 * @note Every mesh in the input needs to be defragmented.
 */
class ScriptContext : public expr::ExpressionContext
{
  public:
    ScriptContext(nt::NodeId currentNode, const NodePacket& input);

    /// @brief Returns the input mesh at a primitive index.
    const geo::Mesh& getInputMesh(size_t primitiveIndex) const;

    /// @brief Returns the reader for the input mesh at a primitive index.
    const MeshAttributeReader& getAttributeReader(size_t primitiveIndex) const;

    /// @brief Adds a warning for the node, skipping one it already holds.
    void addWarning(const String& warning) const;

    /// @brief Returns every warning the script added, in the order they first appeared.
    const std::vector<String>& getWarnings() const { return warnings_; }

  private:
    // One entry per primitive of the input, empty for a primitive that isn't a mesh.
    std::vector<std::shared_ptr<const geo::Mesh>> inputMeshes_;
    std::vector<std::optional<MeshAttributeReader>> attributeReaders_;

    mutable std::mutex warningMutex_;
    mutable std::vector<String> warnings_;
};

} // namespace enzo::script
