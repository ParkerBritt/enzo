#include "Engine/GeometryAlgorithms/AttributeTransfer.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include <cmath>
#include <memory>
#include <vector>

namespace enzo::utils {

namespace {

// A source attribute or group with the destination one its values are written into.
struct AttributePair
{
    std::shared_ptr<const attr::Attribute> source;
    std::shared_ptr<attr::Attribute> dest;
};

// Returns each attribute and group of the source paired with the destination one of the same name
// and type.
std::vector<AttributePair>
getAttributePairs(const geo::Primitive& source, geo::Primitive& dest, attr::AttributeOwner owner)
{
    std::vector<AttributePair> pairs;
    for (const auto& sourceAttribute : source.getAttributes(owner))
    {
        auto destAttribute = dest.getAttribByName(owner, sourceAttribute->getName());
        if (!destAttribute) continue;
        if (destAttribute->getType() != sourceAttribute->getType()) continue;
        pairs.push_back({sourceAttribute, destAttribute});
    }
    for (const auto& sourceGroup : source.getGroups(owner))
    {
        auto destGroup = dest.getGroupByName(owner, sourceGroup->getName());
        if (destGroup) pairs.push_back({sourceGroup, destGroup});
    }
    return pairs;
}

// Returns the source element nearer to where a blend lands.
Offset getNearerSourceOffset(const ElementBlend& elementBlend)
{
    return elementBlend.blend < 0.5 ? elementBlend.sourceOffset0 : elementBlend.sourceOffset1;
}

// Copies each source element's value into its destination element.
template <typename T>
void copyValues(
    const AttributePair& pair,
    std::span<const Offset> sourceOffsets,
    std::span<const Offset> destOffsets
)
{
    attr::AttributeHandleRO<T> sourceHandle(pair.source);
    attr::AttributeHandle<T> destHandle(pair.dest);
    for (size_t elementIndex = 0; elementIndex < sourceOffsets.size(); ++elementIndex)
    {
        const T value = sourceHandle.getValue(sourceOffsets[elementIndex]);
        destHandle.setValue(destOffsets[elementIndex], value);
    }
}

// Writes the value of each blend's nearer source element into its destination element.
template <typename T>
void copyNearerValues(const AttributePair& pair, std::span<const ElementBlend> elementBlends)
{
    attr::AttributeHandleRO<T> sourceHandle(pair.source);
    attr::AttributeHandle<T> destHandle(pair.dest);
    for (const ElementBlend& elementBlend : elementBlends)
    {
        const T value = sourceHandle.getValue(getNearerSourceOffset(elementBlend));
        destHandle.setValue(elementBlend.destOffset, value);
    }
}

// Writes each blend of two source values into its destination element.
template <typename T>
void interpolateValues(const AttributePair& pair, std::span<const ElementBlend> elementBlends)
{
    attr::AttributeHandleRO<T> sourceHandle(pair.source);
    attr::AttributeHandle<T> destHandle(pair.dest);
    for (const ElementBlend& elementBlend : elementBlends)
    {
        const T value0 = sourceHandle.getValue(elementBlend.sourceOffset0);
        const T value1 = sourceHandle.getValue(elementBlend.sourceOffset1);
        const T blended = value0 * (1.0 - elementBlend.blend) + value1 * elementBlend.blend;
        destHandle.setValue(elementBlend.destOffset, blended);
    }
}

// Writes each blend of two source integers into its destination element, rounded to the nearest
// whole number.
void interpolateIntegerValues(const AttributePair& pair, std::span<const ElementBlend> elementBlends)
{
    attr::AttributeHandleRO<intT> sourceHandle(pair.source);
    attr::AttributeHandle<intT> destHandle(pair.dest);
    for (const ElementBlend& elementBlend : elementBlends)
    {
        const intT value0 = sourceHandle.getValue(elementBlend.sourceOffset0);
        const intT value1 = sourceHandle.getValue(elementBlend.sourceOffset1);
        const double blended = value0 * (1.0 - elementBlend.blend) + value1 * elementBlend.blend;
        destHandle.setValue(elementBlend.destOffset, static_cast<intT>(std::llround(blended)));
    }
}

} // namespace

void copyAttributeValues(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    std::span<const Offset> sourceOffsets,
    std::span<const Offset> destOffsets
)
{
    for (const AttributePair& pair : getAttributePairs(source, dest, owner))
    {
        switch (pair.source->getType())
        {
        case attr::AttributeType::intT:
            copyValues<intT>(pair, sourceOffsets, destOffsets);
            break;
        case attr::AttributeType::floatT:
            copyValues<floatT>(pair, sourceOffsets, destOffsets);
            break;
        case attr::AttributeType::vectorT:
            copyValues<Vector3>(pair, sourceOffsets, destOffsets);
            break;
        case attr::AttributeType::boolT:
            copyValues<boolT>(pair, sourceOffsets, destOffsets);
            break;
        case attr::AttributeType::matrixT:
            copyValues<Matrix4>(pair, sourceOffsets, destOffsets);
            break;
        default:
            break;
        }
    }
}

void interpolateAttributeValues(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    std::span<const ElementBlend> elementBlends
)
{
    for (const AttributePair& pair : getAttributePairs(source, dest, owner))
    {
        switch (pair.source->getType())
        {
        case attr::AttributeType::intT:
            interpolateIntegerValues(pair, elementBlends);
            break;
        case attr::AttributeType::floatT:
            interpolateValues<floatT>(pair, elementBlends);
            break;
        case attr::AttributeType::vectorT:
            interpolateValues<Vector3>(pair, elementBlends);
            break;
        case attr::AttributeType::boolT:
            copyNearerValues<boolT>(pair, elementBlends);
            break;
        case attr::AttributeType::matrixT:
            copyNearerValues<Matrix4>(pair, elementBlends);
            break;
        default:
            break;
        }
    }
}

} // namespace enzo::utils
