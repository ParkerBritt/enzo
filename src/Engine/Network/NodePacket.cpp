#include "Engine/Network/NodePacket.h"
#include <cstddef>
#include <stdexcept>

namespace enzo {

// ---
// Transforms::Iterator
// ---

NodePacket::Transforms::Iterator::Iterator(
    std::vector<std::shared_ptr<geo::Primitive>>& primitives,
    TransformClass transformClass,
    size_t primIdx
)
    : primitives_(primitives), transformClass_(transformClass), primIdx_(primIdx)
{
    advance();
}

Transform NodePacket::Transforms::Iterator::operator*() const
{
    return Transform(*curAttrib_, offset_);
}

NodePacket::Transforms::Iterator& NodePacket::Transforms::Iterator::operator++()
{
    ++offset_;
    if (offset_ >= curSize_)
    {
        ++primIdx_;
        offset_ = 0;
        advance();
    }
    return *this;
}

NodePacket::Transforms::Iterator NodePacket::Transforms::Iterator::operator++(int)
{
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

void NodePacket::Transforms::Iterator::advance()
{
    curAttrib_ = nullptr;
    curSize_ = 0;
    while (primIdx_ < primitives_.size())
    {
        auto& prim = primitives_[primIdx_];
        TransformClass primTransformClass = prim->transformType();

        // POINT takes priority: use P attribute if both the query and primitive support it
        if (hasFlag(transformClass_, TransformClass::POINT) &&
            hasFlag(primTransformClass, TransformClass::POINT))
        {
            auto attrib = prim->getAttribByName(attr::AttributeOwner::POINT, "P", true);
            if (attrib && attrib->getSize() > 0)
            {
                curAttrib_ = attrib;
                curSize_ = attrib->getSize();
                return;
            }
        }

        // Fallback: primitive-level transform attribute
        if (hasFlag(transformClass_, TransformClass::PRIMITIVE) &&
            hasFlag(primTransformClass, TransformClass::PRIMITIVE))
        {
            auto attrib = prim->getAttribByName(attr::AttributeOwner::PRIMITIVE, "transform", true);
            if (attrib && attrib->getSize() > 0)
            {
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

void NodePacket::addPrimitive(std::shared_ptr<geo::Primitive> primitive)
{
    primitives_.push_back(std::move(primitive));
}

void NodePacket::attemptMerge(std::shared_ptr<geo::Primitive> newPrim)
{
    if (newPrim->canMerge())
    {
        auto existing = getPrimAtPath(newPrim->getPath());
        if (existing)
        {
            existing->merge(newPrim);
            return;
        }
    }
    else if (getPrimAtPath(newPrim->getPath()))
    {
        newPrim->incrementVersion();
    }
    addPrimitive(newPrim);
}

std::shared_ptr<geo::Primitive> NodePacket::getPrimitive(unsigned int index)
{
    if (index >= primitives_.size())
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    return primitives_.at(index);
}

std::shared_ptr<const geo::Primitive> NodePacket::getPrimitive(unsigned int index) const
{
    if (index >= primitives_.size())
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    return primitives_.at(index);
}

std::shared_ptr<geo::Primitive> NodePacket::getPrimAtPath(const std::string& path)
{
    for (auto& prim : primitives_)
    {
        if (prim->getPath() == path) return prim;
    }
    return nullptr;
}

std::shared_ptr<const geo::Primitive> NodePacket::getPrimAtPath(const std::string& path) const
{
    for (const auto& prim : primitives_)
    {
        if (prim->getPath() == path) return prim;
    }
    return nullptr;
}

std::vector<std::shared_ptr<geo::Primitive>> NodePacket::getPrimitives(geo::PrimType type) const
{
    std::vector<std::shared_ptr<geo::Primitive>> out;
    for (const auto& prim : primitives_)
    {
        if (prim->getType() == type) out.push_back(prim);
    }
    return out;
}

size_t NodePacket::size() const { return primitives_.size(); }

NodePacket NodePacket::deepCopy() const
{
    NodePacket copy;
    for (const auto& prim : primitives_)
        copy.addPrimitive(prim->clone());
    return copy;
}

// TODO: remplace with PrimPath
void NodePacket::removePrim(std::string path)
{
    for (auto it = primitives_.begin(); it != primitives_.end(); it++)
    {
        if ((*it)->getPath() == path)
        {
            primitives_.erase(it);
            return;
        }
    }
}

} // namespace enzo
