#include "Engine/Operator/NodePacket.h"
#include <cstddef>
#include <stdexcept>

// ---
// Transforms::Iterator
// ---

enzo::NodePacket::Transforms::Iterator::Iterator(
    std::vector<std::shared_ptr<geo::Primitive>> &primitives, TransformClass transformClass,
    size_t primIdx)
    : primitives_(primitives), transformClass_(transformClass), primIdx_(primIdx) {
    advance();
}

enzo::Transform enzo::NodePacket::Transforms::Iterator::operator*() const {
    return enzo::Transform(*curAttrib_, offset_);
}

enzo::NodePacket::Transforms::Iterator &enzo::NodePacket::Transforms::Iterator::operator++() {
    ++offset_;
    if (offset_ >= curSize_) {
        ++primIdx_;
        offset_ = 0;
        advance();
    }
    return *this;
}

enzo::NodePacket::Transforms::Iterator enzo::NodePacket::Transforms::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

void enzo::NodePacket::Transforms::Iterator::advance() {
    curAttrib_ = nullptr;
    curSize_ = 0;
    while (primIdx_ < primitives_.size()) {
        auto &prim = primitives_[primIdx_];
        TransformClass primTransformClass = prim->transformType();

        // POINT takes priority: use P attribute if both the query and primitive support it
        if (hasFlag(transformClass_, TransformClass::POINT) &&
            hasFlag(primTransformClass, TransformClass::POINT)) {
            auto attrib = prim->getAttribByName(ga::AttributeOwner::POINT, "P", true);
            if (attrib && attrib->getSize() > 0) {
                curAttrib_ = attrib;
                curSize_ = attrib->getSize();
                return;
            }
        }

        // Fallback: primitive-level transform attribute
        if (hasFlag(transformClass_, TransformClass::PRIMITIVE) &&
            hasFlag(primTransformClass, TransformClass::PRIMITIVE)) {
            auto attrib = prim->getAttribByName(ga::AttributeOwner::PRIMITIVE, "transform", true);
            if (attrib && attrib->getSize() > 0) {
                curAttrib_ = attrib;
                curSize_ = attrib->getSize();
                return;
            }
        }

        ++primIdx_;
    }
}

// ---
// NodePacket
// ---

void enzo::NodePacket::addPrimitive(std::shared_ptr<enzo::geo::Primitive> primitive) {
    primitives_.push_back(std::move(primitive));
}

void enzo::NodePacket::attemptMerge(std::shared_ptr<geo::Primitive> newPrim) {
    if (newPrim->canMerge()) {
        auto existing = getPrimAtPath(newPrim->getPath());
        if (existing) {
            existing->merge(newPrim);
            return;
        }
    } else if (getPrimAtPath(newPrim->getPath())) {
        newPrim->incrementVersion();
    }
    addPrimitive(newPrim);
}

std::shared_ptr<enzo::geo::Primitive> enzo::NodePacket::getPrimitive(unsigned int index) {
    if (index >= primitives_.size())
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    return primitives_.at(index);
}

std::shared_ptr<const enzo::geo::Primitive>
enzo::NodePacket::getPrimitive(unsigned int index) const {
    if (index >= primitives_.size())
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    return primitives_.at(index);
}

std::shared_ptr<enzo::geo::Primitive> enzo::NodePacket::getPrimAtPath(const std::string &path) {
    for (auto &prim : primitives_) {
        if (prim->getPath() == path)
            return prim;
    }
    return nullptr;
}

std::shared_ptr<const enzo::geo::Primitive>
enzo::NodePacket::getPrimAtPath(const std::string &path) const {
    for (const auto &prim : primitives_) {
        if (prim->getPath() == path)
            return prim;
    }
    return nullptr;
}

std::vector<std::shared_ptr<enzo::geo::Primitive>>
enzo::NodePacket::getPrimitives(enzo::geo::PrimType type) const {
    std::vector<std::shared_ptr<enzo::geo::Primitive>> out;
    for (const auto &prim : primitives_) {
        if (prim->getType() == type)
            out.push_back(prim);
    }
    return out;
}

size_t enzo::NodePacket::size() const { return primitives_.size(); }

enzo::NodePacket enzo::NodePacket::deepCopy() const {
    NodePacket copy;
    for (const auto &prim : primitives_)
        copy.addPrimitive(prim->clone());
    return copy;
}
