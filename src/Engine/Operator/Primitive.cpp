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

const size_t geo::Primitive::getNumAttributes(const ga::AttributeOwner owner) const {
    size_t count = 0;
    for (const auto &attribute : getAttributeStore(owner)) {
        if (attribute && !attribute->isPrivate())
            ++count;
    }
    return count;
}

std::weak_ptr<const ga::Attribute> geo::Primitive::getAttributeByIndex(ga::AttributeOwner owner,
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

ga::AttributeHandleInt geo::Primitive::addIntAttribute(ga::AttributeOwner owner, std::string name,
                                                       bool intrinsic) {
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::intT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleInt(newAttribute);
}

ga::AttributeHandleBool geo::Primitive::addBoolAttribute(ga::AttributeOwner owner, std::string name,
                                                         bool intrinsic, bool isPrivate) {
    auto newAttribute =
        std::make_shared<ga::Attribute>(name, ga::AttrType::boolT, intrinsic, isPrivate);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleBool(newAttribute);
}

ga::AttributeHandle<bt::Vector3>
geo::Primitive::addVector3Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic) {
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::vectorT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandle<bt::Vector3>(newAttribute);
}

ga::AttributeHandle<bt::Matrix4>
geo::Primitive::addMatrix4Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic) {
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::matrixT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandle<bt::Matrix4>(newAttribute);
}

ga::attribVector &geo::Primitive::getAttributeStore(const ga::AttributeOwner &owner) {
    switch (owner) {
    case ga::AttributeOwner::POINT:
        return pointAttributes_;
    case ga::AttributeOwner::PRIMITIVE:
        return primitiveAttributes_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const ga::attribVector &
geo::Primitive::getAttributeStore(const ga::AttributeOwner &owner) const {
    switch (owner) {
    case ga::AttributeOwner::POINT:
        return pointAttributes_;
    case ga::AttributeOwner::PRIMITIVE:
        return primitiveAttributes_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

ga::attribVector &geo::Primitive::getGroupStore(const ga::AttributeOwner &owner) {
    switch (owner) {
    case ga::AttributeOwner::POINT:
        return pointGroups_;
    case ga::AttributeOwner::PRIMITIVE:
        return primitiveGroups_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const ga::attribVector &
geo::Primitive::getGroupStore(const ga::AttributeOwner &owner) const {
    switch (owner) {
    case ga::AttributeOwner::POINT:
        return pointGroups_;
    case ga::AttributeOwner::PRIMITIVE:
        return primitiveGroups_;
    default:
        throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

size_t geo::Primitive::getElementCount(const ga::AttributeOwner &owner) const {
    // Primitive owners carry exactly one entry per primitive instance.
    if (owner == ga::AttributeOwner::PRIMITIVE) return 1;

    // For the other owners every attribute in the store shares a length,
    // so the first attribute's size is the canonical element count.
    const auto &store = getAttributeStore(owner);
    if (store.empty()) return 0;
    return store.front()->getSize();
}

ga::AttributeHandleBool geo::Primitive::createGroup(ga::AttributeOwner owner, std::string name) {
    auto newGroup = std::make_shared<ga::Attribute>(name, ga::AttrType::boolT);
    // Match the owner's current element count so existing elements start as non-members.
    newGroup->resize(getElementCount(owner));
    getGroupStore(owner).push_back(newGroup);
    return ga::AttributeHandleBool(newGroup);
}

void geo::Primitive::addToGroup(ga::AttributeOwner owner, const std::string &name,
                                const std::vector<ga::Offset> &offsets) {
    auto group = getGroupByName(owner, name);
    if (!group) throw std::runtime_error("addToGroup: no group named '" + name + "'");
    ga::AttributeHandleBool handle(group);
    for (ga::Offset offset : offsets) {
        handle.setValue(offset, true);
    }
}

std::shared_ptr<ga::Attribute> geo::Primitive::getGroupByName(ga::AttributeOwner owner,
                                                              const std::string &name) {
    for (auto &group : getGroupStore(owner)) {
        if (group && group->getName() == name) return group;
    }
    return nullptr;
}

size_t geo::Primitive::getNumGroups(ga::AttributeOwner owner) const {
    return getGroupStore(owner).size();
}

std::weak_ptr<const ga::Attribute>
geo::Primitive::getGroupByIndex(ga::AttributeOwner owner, unsigned int index) const {
    const auto &store = getGroupStore(owner);
    if (index >= store.size()) {
        throw std::out_of_range("Group index out of range: " + std::to_string(index) +
                                " size: " + std::to_string(store.size()));
    }
    return store[index];
}

bool geo::Primitive::attributeExists(ga::AttributeOwner owner, std::string name) {
    return static_cast<bool>(getAttribByName(owner, name));
}

std::shared_ptr<ga::Attribute> geo::Primitive::getAttribByName(ga::AttributeOwner owner,
                                                               std::string name,
                                                               bool includeIntrinsics) {
    auto &vector = getAttributeStore(owner);
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        std::shared_ptr<ga::Attribute> attribute = (*it);
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

ga::attribVector geo::Primitive::deepCopyAttributes(ga::attribVector originalVector) {
    ga::attribVector copied;
    const size_t sourceSize = originalVector.size();

    copied.reserve(sourceSize);

    for (const std::shared_ptr<ga::Attribute> sourceAttrib : originalVector) {
        if (sourceAttrib) {
            copied.push_back(std::make_shared<ga::Attribute>(*sourceAttrib));
        } else {
            copied.push_back(nullptr);
        }
    }

    return copied;
}
