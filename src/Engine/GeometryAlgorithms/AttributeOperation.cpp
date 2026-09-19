#include "Engine/GeometryAlgorithms/AttributeOperation.h"

#include <algorithm>

namespace enzo::utils {

std::optional<AttributeOperation> getAttributeOperation(const String& name)
{
    if (name == "set") return AttributeOperation::SET;
    if (name == "add") return AttributeOperation::ADD;
    if (name == "subtract") return AttributeOperation::SUBTRACT;
    if (name == "multiply") return AttributeOperation::MULTIPLY;
    if (name == "minimum") return AttributeOperation::MINIMUM;
    if (name == "maximum") return AttributeOperation::MAXIMUM;
    return std::nullopt;
}

floatT applyOperation(AttributeOperation operation, floatT value, floatT newValue)
{
    switch (operation)
    {
    case AttributeOperation::SET:
        return newValue;
    case AttributeOperation::ADD:
        return value + newValue;
    case AttributeOperation::SUBTRACT:
        return value - newValue;
    case AttributeOperation::MULTIPLY:
        return value * newValue;
    case AttributeOperation::MINIMUM:
        return std::min(value, newValue);
    case AttributeOperation::MAXIMUM:
        return std::max(value, newValue);
    }
    return value;
}

Vector3 applyOperation(AttributeOperation operation, const Vector3& value, const Vector3& newValue)
{
    return Vector3(
        applyOperation(operation, value.x(), newValue.x()),
        applyOperation(operation, value.y(), newValue.y()),
        applyOperation(operation, value.z(), newValue.z())
    );
}

void applyOperation(
    AttributeOperation operation,
    attr::AttributeHandle<floatT> handle,
    std::span<const floatT> newValues
)
{
    for (size_t offset = 0; offset < newValues.size(); ++offset)
    {
        const floatT value = handle.getValue(offset);
        handle.setValue(offset, applyOperation(operation, value, newValues[offset]));
    }
}

void applyOperation(
    AttributeOperation operation,
    attr::AttributeHandle<Vector3> handle,
    std::span<const Vector3> newValues
)
{
    for (size_t offset = 0; offset < newValues.size(); ++offset)
    {
        const Vector3 value = handle.getValue(offset);
        handle.setValue(offset, applyOperation(operation, value, newValues[offset]));
    }
}

} // namespace enzo::utils
