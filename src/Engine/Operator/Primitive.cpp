#include "Engine/Operator/Primitive.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include <memory>
#include <stdexcept>
#include <string>

using namespace enzo;

geo::Primitive::Primitive(std::string_view path) : path_(path) {}

geo::Primitive::Primitive(const Primitive &other)
    : pointAttributes_{deepCopyAttributes(other.pointAttributes_)},
      primitiveAttributes_{deepCopyAttributes(other.primitiveAttributes_)},
      pointGroups_{deepCopyAttributes(other.pointGroups_)},
      primitiveGroups_{deepCopyAttributes(other.primitiveGroups_)},
      path_{other.path_} {}

enzo::geo::Primitive &enzo::geo::Primitive::operator=(const enzo::geo::Primitive &rhs) {
    if (this == &rhs)
        return *this;

    pointAttributes_ = deepCopyAttributes(rhs.pointAttributes_);
    primitiveAttributes_ = deepCopyAttributes(rhs.primitiveAttributes_);
    pointGroups_ = deepCopyAttributes(rhs.pointGroups_);
    primitiveGroups_ = deepCopyAttributes(rhs.primitiveGroups_);
    path_ = rhs.path_;

    return *this;
}

const size_t geo::Primitive::getNumAttributes(const attr::AttributeOwner owner) const {
    size_t count = 0;
    for (const auto &attribute : getAttributeStore(owner)) {
        if (attribute && !attribute->isPrivate())
            ++count;
    }
    return count;
}

std::weak_ptr<const attr::Attribute> geo::Primitive::getAttributeByIndex(attr::AttributeOwner owner,
                                                                       unsigned int index) const {
    const auto &attribStore = getAttributeStore(owner);
    unsigned int visibleIndex = 0;
    for (const auto &attribute : attribStore) {
        if (!attribute || attribute->isPrivate())
            continue;
        if (visibleIndex == index)
            return attribute;
        ++visibleIndex;
    }
    throw std::out_of_range("Attribute index out of range: " + std::to_string(index) +
                            " visible size: " + std::to_string(visibleIndex) + "\n");
}

attr::AttributeHandleInt geo::Primitive::addIntAttribute(attr::AttributeOwner owner, std::string name,
                                                       bool intrinsic) {
    auto newAttribute = std::make_shared<attr::Attribute>(name, attr::AttrType::intT, intrinsic);
    newAttribute->resize(getElementCount(owner));
    getAttributeStore(owner).push_back(newAttribute);
    return attr::AttributeHandleInt(newAttribute);
}

attr::AttributeHandleBool geo::Primitive::addBoolAttribute(attr::AttributeOwner owner, std::string name,
                                                         bool intrinsic, bool isPrivate) {
    auto newAttribute =
        std::make_shared<attr::Attribute>(name, attr::AttrType::boolT, intrinsic, isPrivate);
    newAttribute->resize(getElementCount(owner));
    getAttributeStore(owner).push_back(newAttribute);
    return attr::AttributeHandleBool(newAttribute);
}

attr::AttributeHandle<bt::Vector3>
geo::Primitive::addVector3Attribute(attr::AttributeOwner owner, std::string name, bool intrinsic) {
    auto newAttribute = std::make_shared<attr::Attribute>(name, attr::AttrType::vectorT, intrinsic);
    newAttribute->resize(getElementCount(owner));
    getAttributeStore(owner).push_back(newAttribute);
    return attr::AttributeHandle<bt::Vector3>(newAttribute);
}

attr::AttributeHandle<bt::Matrix4>
geo::Primitive::addMatrix4Attribute(attr::AttributeOwner owner, std::string name, bool intrinsic) {
    auto newAttribute = std::make_shared<attr::Attribute>(name, attr::AttrType::matrixT, intrinsic);
    newAttribute->resize(getElementCount(owner));
    getAttributeStore(owner).push_back(newAttribute);
    return attr::AttributeHandle<bt::Matrix4>(newAttribute);
}

attr::attribVector &geo::Primitive::getAttributeStore(const attr::AttributeOwner &owner) {
    switch (owner) {
    case attr::AttributeOwner::POINT:
        return pointAttributes_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveAttributes_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const attr::attribVector &
geo::Primitive::getAttributeStore(const attr::AttributeOwner &owner) const {
    switch (owner) {
    case attr::AttributeOwner::POINT:
        return pointAttributes_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveAttributes_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

attr::attribVector &geo::Primitive::getGroupStore(const attr::AttributeOwner &owner) {
    switch (owner) {
    case attr::AttributeOwner::POINT:
        return pointGroups_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveGroups_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const attr::attribVector &
geo::Primitive::getGroupStore(const attr::AttributeOwner &owner) const {
    switch (owner) {
    case attr::AttributeOwner::POINT:
        return pointGroups_;
    case attr::AttributeOwner::PRIMITIVE:
        return primitiveGroups_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

size_t geo::Primitive::getElementCount(const attr::AttributeOwner &owner) const {
    // Primitive owners carry exactly one entry per primitive instance.
    if (owner == attr::AttributeOwner::PRIMITIVE) return 1;

    // For the other owners every attribute in the store shares a length,
    // so the first attribute's size is the canonical element count.
    const auto &store = getAttributeStore(owner);
    if (store.empty()) return 0;
    return store.front()->getSize();
}

attr::AttributeHandleBool geo::Primitive::createGroup(attr::AttributeOwner owner, std::string name) {
    auto newGroup = std::make_shared<attr::Attribute>(name, attr::AttrType::boolT);
    // Match the owner's current element count so existing elements start as non-members.
    newGroup->resize(getElementCount(owner));
    getGroupStore(owner).push_back(newGroup);
    return attr::AttributeHandleBool(newGroup);
}

void geo::Primitive::addToGroup(attr::AttributeOwner owner, const std::string &name,
                                const std::vector<attr::Offset> &offsets) {
    auto group = getGroupByName(owner, name);
    if (!group) throw std::runtime_error("addToGroup: no group named '" + name + "'");
    attr::AttributeHandleBool handle(group);
    for (attr::Offset offset : offsets) {
        handle.setValue(offset, true);
    }
}

std::shared_ptr<attr::Attribute> geo::Primitive::getGroupByName(attr::AttributeOwner owner,
                                                              const std::string &name) const {
    for (const auto &group : getGroupStore(owner)) {
        if (group && group->getName() == name) return group;
    }
    return nullptr;
}

size_t geo::Primitive::getNumGroups(attr::AttributeOwner owner) const {
    return getGroupStore(owner).size();
}

std::weak_ptr<const attr::Attribute>
geo::Primitive::getGroupByIndex(attr::AttributeOwner owner, unsigned int index) const {
    const auto &store = getGroupStore(owner);
    if (index >= store.size()) {
        throw std::out_of_range("Group index out of range: " + std::to_string(index) +
                                " size: " + std::to_string(store.size()));
    }
    return store[index];
}

bool geo::Primitive::attributeExists(attr::AttributeOwner owner, std::string name) {
    return static_cast<bool>(getAttribByName(owner, name));
}

std::shared_ptr<attr::Attribute> geo::Primitive::getAttribByName(attr::AttributeOwner owner,
                                                               std::string name,
                                                               bool includeIntrinsics) {
    auto &vector = getAttributeStore(owner);
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        std::shared_ptr<attr::Attribute> attribute = (*it);
        if (attribute->getName() == name) {
            if (!includeIntrinsics && attribute->isIntrinsic())
                continue;
            return attribute;
        }
    }
    return nullptr;
}

std::shared_ptr<const attr::Attribute> geo::Primitive::getAttribByName(attr::AttributeOwner owner,
                                                                     std::string name,
                                                                     bool includeIntrinsics) const {
    const auto &vector = getAttributeStore(owner);
    for (const std::shared_ptr<attr::Attribute>& attribute : vector) {
        if (attribute->getName() == name) {
            if (!includeIntrinsics && attribute->isIntrinsic())
                continue;
            return attribute;
        }
    }
    return nullptr;
}

void geo::Primitive::incrementVersion() {
    // TODO: temporary placeholder. Replace once PrimPath class is implemented
    path_ += "_02";
}

attr::attribVector geo::Primitive::deepCopyAttributes(attr::attribVector originalVector) {
    attr::attribVector copied;
    const size_t sourceSize = originalVector.size();

    copied.reserve(sourceSize);

    for (const std::shared_ptr<attr::Attribute> sourceAttrib : originalVector) {
        if (sourceAttrib) {
            copied.push_back(std::make_shared<attr::Attribute>(*sourceAttrib));
        } else {
            copied.push_back(nullptr);
        }
    }

    return copied;
}
