#pragma once
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include <optional>
#include <span>

namespace enzo::utils {

/// @brief How a new value combines with the value already on an attribute.
enum class AttributeOperation
{
    SET,
    ADD,
    SUBTRACT,
    MULTIPLY,
    MINIMUM,
    MAXIMUM,
};

/// @brief Returns the operation a dropdown value names, or nothing when it names none.
std::optional<AttributeOperation> getAttributeOperation(const String& name);

/// @brief Returns the value after the new value is combined into it.
floatT applyOperation(AttributeOperation operation, floatT value, floatT newValue);

/// @brief Returns the vector after the new vector is combined into each axis.
Vector3 applyOperation(AttributeOperation operation, const Vector3& value, const Vector3& newValue);

/// @brief Combines each new value into the attribute value at the same offset.
void applyOperation(
    AttributeOperation operation,
    attr::AttributeHandle<floatT> handle,
    std::span<const floatT> newValues
);

/// @brief Combines each new vector into the attribute vector at the same offset.
void applyOperation(
    AttributeOperation operation,
    attr::AttributeHandle<Vector3> handle,
    std::span<const Vector3> newValues
);

} // namespace enzo::utils
