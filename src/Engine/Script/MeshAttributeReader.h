#pragma once
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include <array>
#include <string_view>
#include <variant>
#include <vector>

namespace enzo::geo {
class Mesh;
}

namespace enzo::script {

/// @brief A read handle for an attribute, empty for a type scripts can't read.
using ScriptAttributeHandle = std::variant<
    std::monostate,
    attr::AttributeHandleRO<floatT>,
    attr::AttributeHandleRO<intT>,
    attr::AttributeHandleRO<Vector3>,
    attr::AttributeHandleRO<boolT>>;

/// @brief An attribute of the mesh that a script reads by name.
struct ScriptAttribute
{
    String name;
    attr::AttributeType type;
    ScriptAttributeHandle handle;
};

/**
 * @brief A mesh's attributes by name, with a read handle for each.
 *
 * @code
 * const MeshAttributeReader reader(mesh);
 * const ScriptAttribute* height = reader.getAttribute(attr::AttributeOwner::POINT, "height");
 * @endcode
 *
 * @note The mesh needs to be defragmented, so an element's index is its offset.
 */
class MeshAttributeReader
{
  public:
    explicit MeshAttributeReader(const geo::Mesh& mesh);

    /// @brief Returns whether an owner has an element at an index.
    bool hasElement(attr::AttributeOwner owner, intT index) const;

    /// @brief Returns the attribute with a name on an owner.
    /// @return The attribute, or null when the owner has none with that name.
    const ScriptAttribute* getAttribute(attr::AttributeOwner owner, std::string_view name) const;

  private:
    static constexpr size_t ownerCount = 4;

    std::array<Offset, ownerCount> elementCounts_;
    std::array<std::vector<ScriptAttribute>, ownerCount> attributes_;
};

} // namespace enzo::script
