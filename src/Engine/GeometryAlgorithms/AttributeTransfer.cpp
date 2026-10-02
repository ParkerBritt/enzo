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

// Returns the mix of two values at a blend between zero and one.
template <typename T> T blendValues(const T& value0, const T& value1, double blend)
{
    return value0 * (1.0 - blend) + value1 * blend;
}

// Returns the mix of two integers, rounded to the nearest whole number.
intT blendValues(intT value0, intT value1, double blend)
{
    return static_cast<intT>(std::llround(value0 * (1.0 - blend) + value1 * blend));
}

// Returns the value nearer to where the blend lands, for types that can't be mixed.
boolT blendValues(boolT value0, boolT value1, double blend) { return blend < 0.5 ? value0 : value1; }

Matrix4 blendValues(const Matrix4& value0, const Matrix4& value1, double blend)
{
    return blend < 0.5 ? value0 : value1;
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
        destHandle.setValue(elementBlend.destOffset, blendValues(value0, value1, elementBlend.blend));
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
        attr::visitType(pair.source->getType(), [&]<typename Value>() {
            copyValues<Value>(pair, sourceOffsets, destOffsets);
        });
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
        attr::visitType(pair.source->getType(), [&]<typename Value>() {
            interpolateValues<Value>(pair, elementBlends);
        });
    }
}

} // namespace enzo::utils
