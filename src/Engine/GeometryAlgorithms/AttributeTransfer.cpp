#include "Engine/GeometryAlgorithms/AttributeTransfer.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include <cmath>
#include <memory>
#include <tbb/parallel_for.h>
#include <type_traits>
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
void copyAttributeValues(
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
void interpolateAttributeValues(
    const AttributePair& pair,
    std::span<const ElementBlend> elementBlends
)
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

// Copies the first valueCount source values copyCount times, one block after another from
// destStart.
template <typename T>
void copyAttributeValuesRepeatedly(
    const AttributePair& pair,
    Offset valueCount,
    Offset destStart,
    Offset copyCount
)
{
    const attr::AttributeHandleRO<T> sourceHandle(pair.source);
    attr::AttributeHandle<T> destHandle(pair.dest);

    const auto writeCopy = [&](Offset copyIndex) {
        const Offset copyStart = destStart + copyIndex * valueCount;
        for (Offset valueIndex = 0; valueIndex < valueCount; ++valueIndex)
            destHandle.setValue(copyStart + valueIndex, sourceHandle.getValue(valueIndex));
    };

    // Writes bools on one thread, since their storage packs neighbouring values into shared bytes.
    if constexpr (std::is_same_v<T, boolT>)
    {
        for (Offset copyIndex = 0; copyIndex < copyCount; ++copyIndex)
            writeCopy(copyIndex);
        return;
    }
    tbb::parallel_for(Offset{0}, copyCount, writeCopy);
}

} // namespace

void copyAttributes(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    std::span<const Offset> sourceOffsets,
    std::span<const Offset> destOffsets
)
{
    dest.addAttributesFrom(source, owner);
    for (const AttributePair& pair : getAttributePairs(source, dest, owner))
    {
        attr::visitType(pair.source->getType(), [&]<typename Value>() {
            copyAttributeValues<Value>(pair, sourceOffsets, destOffsets);
        });
    }
}

void interpolateAttributes(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    std::span<const ElementBlend> elementBlends
)
{
    dest.addAttributesFrom(source, owner);
    for (const AttributePair& pair : getAttributePairs(source, dest, owner))
    {
        attr::visitType(pair.source->getType(), [&]<typename Value>() {
            interpolateAttributeValues<Value>(pair, elementBlends);
        });
    }
}

void copyAttributesRepeatedly(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    Offset valueCount,
    Offset destStart,
    Offset copyCount
)
{
    dest.addAttributesFrom(source, owner);
    for (const AttributePair& pair : getAttributePairs(source, dest, owner))
    {
        attr::visitType(pair.source->getType(), [&]<typename Value>() {
            copyAttributeValuesRepeatedly<Value>(pair, valueCount, destStart, copyCount);
        });
    }
}

} // namespace enzo::utils
