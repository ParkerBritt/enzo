#pragma once
#include "Engine/Serializer/ParameterSerializable.h"
#include <string>
#include <vector>

namespace enzo::nt {

/**
 * @brief A tab menu entry that creates a node of another type with its own starting values.
 *
 * A node created from the "enzo::mountain" alias is an "enzo::attributeNoise" node whose
 * parameters start at the values the alias gives them.
 *
 * @note The node keeps no record of the alias, so its parameters reset to the defaults of
 * its own type.
 */
struct NodeAlias
{
    std::string internalName;

    /// @brief The namespace the alias was published under, such as "enzo".
    std::string typeNamespace;

    std::string displayName;

    /// @brief The words the tab menu searches on.
    std::vector<std::string> tags;

    /// @brief The full name of the node type the alias creates, such as "enzo::attributeNoise".
    std::string aliasedType;

    /// @brief The starting values set on each node the alias creates.
    std::vector<ParameterSerializable> parameterValues;

    /// @brief Returns the name that uniquely identifies the alias, such as "enzo::mountain".
    std::string getFullName() const { return typeNamespace + "::" + internalName; }

    /// @brief Returns the label shown in the tab menu, such as "Mountain".
    const std::string& getLabel() const { return displayName; }
};

} // namespace enzo::nt
