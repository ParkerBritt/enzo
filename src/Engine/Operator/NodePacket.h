#pragma once
#include "Engine/Operator/Primitive.h"
#include <memory>
#include <vector>

namespace enzo
{
class NodePacket
{
public:
    void addPrimitive(std::shared_ptr<enzo::geo::Primitive> primitive);
    std::shared_ptr<enzo::geo::Primitive> getPrimitive(unsigned int index);
    std::shared_ptr<const enzo::geo::Primitive> getPrimitive(unsigned int index) const;
    size_t size() const;
    NodePacket deepCopy() const;

private:
    std::vector<std::shared_ptr<enzo::geo::Primitive>> primitives_;
};
}
