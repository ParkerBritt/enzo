#include "Engine/Primitives/Primitive.h"
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

namespace enzo {

geo::Primitive::Primitive(std::string_view path) : path_(path) {}

geo::Primitive::Primitive(const Primitive& other)
    : pointAttributes_{deepCopyAttributes(other.pointAttributes_)},
      primitiveAttributes_{deepCopyAttributes(other.primitiveAttributes_)},
      pointGroups_{deepCopyAttributes(other.pointGroups_)},
      primitiveGroups_{deepCopyAttributes(other.primitiveGroups_)}, path_{other.path_}
{
}

geo::Primitive& geo::Primitive::operator=(const geo::Primitive& rhs)
{
    if (this == &rhs) return *this;

    pointAttributes_ = deepCopyAttributes(rhs.pointAttributes_);
    primitiveAttributes_ = deepCopyAttributes(rhs.primitiveAttributes_);
    pointGroups_ = deepCopyAttributes(rhs.pointGroups_);
    primitiveGroups_ = deepCopyAttributes(rhs.primitiveGroups_);
    path_ = rhs.path_;

    return *this;
}

const size_t geo::Primitive::getNumAttributes(const attr::AttributeOwner owner) const
{
    size_t count = 0;
    for (const auto& attribute : getAttributeStore(owner))
    {
        if (attribute && !attribute->isPrivate()) ++count;
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
        if (!attribute || attribute->isPrivate()) continue;
        if (visibleIndex == index) return attribute;
        ++visibleIndex;
    }
    throw std::out_of_range(
        "Attribute index out of range: " + std::to_string(index) +
        " visible size: " + std::to_string(visibleIndex) + "\n"
    );
}

namespace {

/**
 * @brief Returns the attribute of this name and type from the store, adding one when
 * the name is free.
 *
 * @note An attribute of another type under the same name is replaced, dropping its
 *       values. Intrinsic and ordinary attributes are matched separately.
 */
std::shared_ptr<attr::Attribute> addToStore(
    attr::attribVector& store,
    size_t elementCount,
    const std::string& name,
    attr::AttributeType type,
    bool intrinsic,
    bool isPrivate
)
{
    const auto holdsTheName = [&](const std::shared_ptr<attr::Attribute>& stored) {
        return stored && stored->getName() == name && stored->isIntrinsic() == intrinsic;
    };
    const auto takenSlot = std::ranges::find_if(store, holdsTheName);
    const bool nameTaken = takenSlot != store.end();

    if (nameTaken && (*takenSlot)->getType() == type) return *takenSlot;

    auto newAttribute = std::make_shared<attr::Attribute>(name, type, intrinsic, isPrivate);
    // Match the owner's element count so existing elements get a value.
    newAttribute->resize(elementCount);

    if (nameTaken) *takenSlot = newAttribute;
    else store.push_back(newAttribute);

    return newAttribute;
}

} // namespace

std::shared_ptr<attr::Attribute> geo::Primitive::addAttribute(
    attr::AttributeOwner owner,
    std::string name,
    attr::AttributeType type,
    bool intrinsic,
    bool isPrivate
)
{
    return addToStore(
        getAttributeStore(owner), getElementCount(owner), name, type, intrinsic, isPrivate
    );
}

attr::AttributeHandleInt
geo::Primitive::addIntAttribute(attr::AttributeOwner owner, std::string name, bool intrinsic)
{
    return attr::AttributeHandleInt(
        addAttribute(owner, std::move(name), attr::AttrType::intT, intrinsic)
    );
}

attr::AttributeHandleFloat
geo::Primitive::addFloatAttribute(attr::AttributeOwner owner, std::string name, bool intrinsic)
{
    return attr::AttributeHandleFloat(
        addAttribute(owner, std::move(name), attr::AttrType::floatT, intrinsic)
    );
}

attr::AttributeHandleBool geo::Primitive::addBoolAttribute(
    attr::AttributeOwner owner,
    std::string name,
    bool intrinsic,
    bool isPrivate
)
{
    return attr::AttributeHandleBool(
        addAttribute(owner, std::move(name), attr::AttrType::boolT, intrinsic, isPrivate)
    );
}

attr::AttributeHandle<Vector3>
geo::Primitive::addVector3Attribute(attr::AttributeOwner owner, std::string name, bool intrinsic)
{
    return attr::AttributeHandle<Vector3>(
        addAttribute(owner, std::move(name), attr::AttrType::vectorT, intrinsic)
    );
}

attr::AttributeHandle<Matrix4>
geo::Primitive::addMatrix4Attribute(attr::AttributeOwner owner, std::string name, bool intrinsic)
{
    return attr::AttributeHandle<Matrix4>(
        addAttribute(owner, std::move(name), attr::AttrType::matrixT, intrinsic)
    );
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
    return attr::AttributeHandleBool(addToStore(
        getGroupStore(owner), getElementCount(owner), name, attr::AttrType::boolT, false, false
    ));
}

void geo::Primitive::addToGroup(
    attr::AttributeOwner owner,
    const std::string& name,
    const std::vector<Offset>& offsets
)
{
    auto group = getGroupByName(owner, name);
    if (!group) throw std::runtime_error("addToGroup: no group named '" + name + "'");
    attr::AttributeHandleBool handle(group);
    for (Offset offset : offsets)
    {
        handle.setValue(offset, true);
    }
}

std::shared_ptr<attr::Attribute>
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

bool geo::Primitive::attributeExists(attr::AttributeOwner owner, std::string name)
{
    return static_cast<bool>(getAttribByName(owner, name));
}

std::shared_ptr<attr::Attribute> geo::Primitive::getAttribByName(
    attr::AttributeOwner owner,
    std::string name,
    bool includeIntrinsics
)
{
    auto& vector = getAttributeStore(owner);
    for (auto it = vector.begin(); it != vector.end(); ++it)
    {
        std::shared_ptr<attr::Attribute> attribute = (*it);
        if (attribute->getName() == name)
        {
            if (!includeIntrinsics && attribute->isIntrinsic()) continue;
            return attribute;
        }
    }
    return nullptr;
}

std::shared_ptr<const attr::Attribute> geo::Primitive::getAttribByName(
    attr::AttributeOwner owner,
    std::string name,
    bool includeIntrinsics
) const
{
    const auto& vector = getAttributeStore(owner);
    for (const std::shared_ptr<attr::Attribute>& attribute : vector)
    {
        if (attribute->getName() == name)
        {
            if (!includeIntrinsics && attribute->isIntrinsic()) continue;
            return attribute;
        }
    }
    return nullptr;
}

void geo::Primitive::incrementVersion()
{
    // TODO: temporary placeholder. Replace once PrimPath class is implemented
    path_ += "_02";
}

attr::attribVector geo::Primitive::deepCopyAttributes(attr::attribVector originalVector)
{
    attr::attribVector copied;
    const size_t sourceSize = originalVector.size();

    copied.reserve(sourceSize);

    for (const std::shared_ptr<attr::Attribute> sourceAttrib : originalVector)
    {
        if (sourceAttrib)
        {
            copied.push_back(std::make_shared<attr::Attribute>(*sourceAttrib));
        }
        else
        {
            copied.push_back(nullptr);
        }
    }

    return copied;
}

} // namespace enzo
