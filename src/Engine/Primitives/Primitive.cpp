#include "Engine/Primitives/Primitive.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace enzo {

geo::Primitive::Primitive(std::string_view path) : path_(path) {}

const size_t geo::Primitive::getNumAttributes(const attr::AttributeOwner owner) const
{
    size_t count = 0;
    for (const auto& attribute : getAttributeStore(owner))
    {
        if (attribute && !attribute->isInternal()) ++count;
    }
    return count;
}

std::weak_ptr<const attr::Attribute>
geo::Primitive::getAttributeByIndex(attr::AttributeOwner owner, unsigned int index) const
{
    const auto& attribStore = getAttributeStore(owner);
    unsigned int visibleIndex = 0;
    for (const auto& attribute : attribStore)
    {
        if (!attribute || attribute->isInternal()) continue;
        if (visibleIndex == index) return attribute;
        ++visibleIndex;
    }
    throw std::out_of_range(
        "Attribute index out of range: " + std::to_string(index) +
        " visible size: " + std::to_string(visibleIndex) + "\n"
    );
}

std::vector<std::shared_ptr<const attr::Attribute>>
geo::Primitive::getAttributes(attr::AttributeOwner owner, bool includeIntrinsics) const
{
    std::vector<std::shared_ptr<const attr::Attribute>> attributes;
    for (const auto& attribute : getAttributeStore(owner))
    {
        if (!attribute || attribute->isInternal()) continue;
        if (!includeIntrinsics && attribute->isIntrinsic()) continue;
        attributes.push_back(attribute);
    }
    return attributes;
}

namespace {

/**
 * @brief Returns the index of the attribute of this name and type, adding one when the name
 * is free.
 *
 * @return The index, or nothing when the name belongs to an internal attribute or to an
 *         intrinsic of another type.
 * @note An ordinary attribute of another type under the same name is replaced,
 *       dropping its values.
 */
std::optional<size_t> addToStore(
    attr::attribVector& store,
    size_t elementCount,
    const std::string& name,
    attr::AttributeType type,
    bool intrinsic,
    bool isInternal
)
{
    const auto holdsTheName = [&](const std::shared_ptr<attr::Attribute>& stored) {
        return stored && stored->getName() == name;
    };
    const auto taken = std::ranges::find_if(store, holdsTheName);
    const bool nameTaken = taken != store.end();
    const size_t takenIndex = taken - store.begin();

    if (nameTaken)
    {
        const std::shared_ptr<attr::Attribute>& stored = *taken;
        const bool typeMatches = stored->getType() == type;
        if (stored->isInternal() && !isInternal) return std::nullopt;
        if (typeMatches) return takenIndex;
        if (stored->isIntrinsic()) return std::nullopt;
    }

    auto newAttribute = std::make_shared<attr::Attribute>(name, type, intrinsic, isInternal);
    // Match the owner's element count so existing elements get a value.
    newAttribute->resize(elementCount);

    if (nameTaken)
    {
        *taken = newAttribute;
        return takenIndex;
    }
    store.push_back(newAttribute);
    return store.size() - 1;
}

} // namespace

std::shared_ptr<const attr::Attribute> geo::Primitive::addAttribute(
    attr::AttributeOwner owner,
    std::string name,
    attr::AttributeType type,
    bool intrinsic,
    bool isInternal
)
{
    std::shared_ptr<const attr::Attribute> attribute =
        tryAddAttribute(owner, name, type, intrinsic, isInternal);
    if (!attribute)
        throw std::runtime_error("The attribute " + name + " can't be added as this type.");
    return attribute;
}

std::shared_ptr<const attr::Attribute> geo::Primitive::tryAddAttribute(
    attr::AttributeOwner owner,
    std::string name,
    attr::AttributeType type,
    bool intrinsic,
    bool isInternal
)
{
    attr::attribVector& attributes = getAttributeStore(owner);
    std::optional<size_t> index =
        addToStore(attributes, getElementCount(owner), name, type, intrinsic, isInternal);
    if (!index) return nullptr;
    return attributes[*index];
}

attr::Attribute& geo::Primitive::unshareAttribute(attr::attribVector& attributes, size_t index)
{
    std::shared_ptr<attr::Attribute>& attribute = attributes[index];
    if (attribute.use_count() > 1) attribute = std::make_shared<attr::Attribute>(*attribute);
    return *attribute;
}

std::vector<attr::Attribute*>
geo::Primitive::getWritableAttributesAndGroups(attr::AttributeOwner owner)
{
    std::vector<attr::Attribute*> attributes;
    for (attr::attribVector* store : {&getAttributeStore(owner), &getGroupStore(owner)})
    {
        for (size_t index = 0; index < store->size(); ++index)
        {
            if ((*store)[index]) attributes.push_back(&unshareAttribute(*store, index));
        }
    }
    return attributes;
}

attr::attribVector& geo::Primitive::getAttributeStore(const attr::AttributeOwner& owner)
{
    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return pointAttributes_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveAttributes_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const attr::attribVector& geo::Primitive::getAttributeStore(const attr::AttributeOwner& owner) const
{
    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return pointAttributes_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveAttributes_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

attr::attribVector& geo::Primitive::getGroupStore(const attr::AttributeOwner& owner)
{
    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return pointGroups_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveGroups_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const attr::attribVector& geo::Primitive::getGroupStore(const attr::AttributeOwner& owner) const
{
    switch (owner)
    {
    case attr::AttributeOwner::POINT:
        return pointGroups_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveGroups_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

size_t geo::Primitive::getElementCount(const attr::AttributeOwner& owner) const
{
    // Primitive owners carry exactly one entry per primitive instance.
    if (owner == attr::AttributeOwner::PRIMITIVE) return 1;

    // For the other owners every attribute in the store shares a length,
    // so the first attribute's size is the canonical element count.
    const auto& store = getAttributeStore(owner);
    if (store.empty()) return 0;
    return store.front()->getSize();
}

attr::AttributeHandleBool geo::Primitive::addGroup(attr::AttributeOwner owner, std::string name)
{
    attr::attribVector& groups = getGroupStore(owner);
    const size_t index = *addToStore(
        groups, getElementCount(owner), name, attr::AttrType::boolT, false, false
    );
    return attr::AttributeHandleBool(unshareAttribute(groups, index));
}

void geo::Primitive::addToGroup(
    attr::AttributeOwner owner,
    const std::string& name,
    const std::vector<Offset>& offsets
)
{
    std::optional<attr::AttributeHandleBool> group = getGroupWriteHandle(owner, name);
    if (!group) throw std::runtime_error("addToGroup: no group named '" + name + "'");
    for (Offset offset : offsets)
    {
        group->setValue(offset, true);
    }
}

std::optional<attr::AttributeHandleBool>
geo::Primitive::getGroupWriteHandle(attr::AttributeOwner owner, std::string_view name)
{
    attr::attribVector& groups = getGroupStore(owner);
    for (size_t index = 0; index < groups.size(); ++index)
    {
        const bool isTheGroup = groups[index] && groups[index]->getName() == name;
        if (isTheGroup) return attr::AttributeHandleBool(unshareAttribute(groups, index));
    }
    return std::nullopt;
}

std::shared_ptr<const attr::Attribute>
geo::Primitive::getGroupByName(attr::AttributeOwner owner, const std::string& name) const
{
    for (const auto& group : getGroupStore(owner))
    {
        if (group && group->getName() == name) return group;
    }
    return nullptr;
}

size_t geo::Primitive::getNumGroups(attr::AttributeOwner owner) const
{
    return getGroupStore(owner).size();
}

std::weak_ptr<const attr::Attribute>
geo::Primitive::getGroupByIndex(attr::AttributeOwner owner, unsigned int index) const
{
    const auto& store = getGroupStore(owner);
    if (index >= store.size())
    {
        throw std::out_of_range(
            "Group index out of range: " + std::to_string(index) +
            " size: " + std::to_string(store.size())
        );
    }
    return store[index];
}

std::vector<std::shared_ptr<const attr::Attribute>>
geo::Primitive::getGroups(attr::AttributeOwner owner) const
{
    const auto& store = getGroupStore(owner);
    return {store.begin(), store.end()};
}

void geo::Primitive::addAttributesFrom(const Primitive& source, attr::AttributeOwner owner)
{
    for (const auto& sourceAttribute : source.getAttributes(owner))
    {
        if (attributeExists(owner, sourceAttribute->getName())) continue;
        addAttribute(owner, sourceAttribute->getName(), sourceAttribute->getType());
    }
    for (const auto& sourceGroup : source.getGroups(owner))
        addGroup(owner, sourceGroup->getName());
}

bool geo::Primitive::attributeExists(attr::AttributeOwner owner, std::string_view name) const
{
    return static_cast<bool>(getAttribByName(owner, name));
}

std::optional<size_t> geo::Primitive::getAttributeIndex(
    attr::AttributeOwner owner,
    std::string_view name,
    bool includeIntrinsics
) const
{
    const attr::attribVector& attributes = getAttributeStore(owner);
    for (size_t index = 0; index < attributes.size(); ++index)
    {
        const std::shared_ptr<attr::Attribute>& attribute = attributes[index];
        if (!attribute || attribute->getName() != name) continue;
        if (!includeIntrinsics && attribute->isIntrinsic()) continue;
        return index;
    }
    return std::nullopt;
}

std::shared_ptr<const attr::Attribute> geo::Primitive::getAttribByName(
    attr::AttributeOwner owner,
    std::string_view name,
    bool includeIntrinsics
) const
{
    std::optional<size_t> index = getAttributeIndex(owner, name, includeIntrinsics);
    if (!index) return nullptr;
    return getAttributeStore(owner)[*index];
}

} // namespace enzo
