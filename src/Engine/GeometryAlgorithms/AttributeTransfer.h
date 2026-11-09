#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Primitives/Primitive.h"
#include <span>

namespace enzo::utils {

/// @brief An element whose attribute values are a blend of two source elements.
struct ElementBlend
{
    Offset sourceOffset0;
    Offset sourceOffset1;
    /// @brief How far from the first source element toward the second, from 0 to 1.
    double blend;
    Offset destOffset;
};

/**
 * @brief Copies the attribute values and group membership of each source element into the
 * destination element at the same place in the list.
 *
 * @note Attributes and groups are matched by name. One missing on the destination is added first,
 * and one holding a different type there is skipped.
 */
void copyAttributes(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    std::span<const Offset> sourceOffsets,
    std::span<const Offset> destOffsets
);

/**
 * @brief Writes a blend of two source elements' attribute values and group membership into each
 * destination element.
 *
 * @note Integers round to the nearest whole number. Booleans, matrices and group membership take
 * the value of whichever source element is nearer. Attributes are matched and added as in
 * copyAttributes.
 */
void interpolateAttributes(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    std::span<const ElementBlend> elementBlends
);

/**
 * @brief Copies the first valueCount attribute values and group membership of the source into
 * the destination copyCount times, one block after another from destStart.
 *
 * With valueCount 2, destStart 4 and copyCount 2, source values 0 and 1 land on 4, 5, 6 and 7.
 *
 * @note Attributes are matched and added as in copyAttributes. The source may be the destination
 * when the blocks sit past its first valueCount values.
 */
void copyAttributesRepeatedly(
    const geo::Primitive& source,
    geo::Primitive& dest,
    attr::AttributeOwner owner,
    Offset valueCount,
    Offset destStart,
    Offset copyCount
);

} // namespace enzo::utils
