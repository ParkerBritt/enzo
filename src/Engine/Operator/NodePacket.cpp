#include "Engine/Operator/NodePacket.h"
#include <stdexcept>

void enzo::NodePacket::addPrimitive(enzo::geo::Primitive primitive)
{
    primitives_.push_back(std::move(primitive));
}

enzo::geo::Primitive& enzo::NodePacket::getPrimitive(unsigned int index)
{
    if(index >= primitives_.size())
    {
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    }
    return primitives_.at(index);
}

const enzo::geo::Primitive& enzo::NodePacket::getPrimitive(unsigned int index) const
{
    if(index >= primitives_.size())
    {
        throw std::out_of_range("NodePacket::getPrimitive index out of range");
    }
    return primitives_.at(index);
}

size_t enzo::NodePacket::size() const
{
    return primitives_.size();
}
