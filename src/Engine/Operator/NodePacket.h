#pragma once
#include "Engine/Operator/Primitive.h"

namespace enzo
{
class NodePacket
{
public:
    void addPrimitive(enzo::geo::Primitive primitive);
    enzo::geo::Primitive& getPrimitive(unsigned int index);
    const enzo::geo::Primitive& getPrimitive(unsigned int index) const;
    size_t size() const;

private:
    std::vector<enzo::geo::Primitive> primitives_;
};
}
