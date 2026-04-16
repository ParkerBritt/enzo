#pragma once

#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"

namespace enzo {
class Point {
  public:
    Point(ga::attribVector &attribute, ga::Offset offset)
        : attributes_(attribute), offset_(offset) {}

    bt::Vector3 getPosition() { return bt::Vector3(0, 0, 0); };
    bt::Matrix4 getTransform() { return bt::Matrix4(); };

  private:
    ga::attribVector &attributes_;
    ga::Offset offset_;
};
} // namespace enzo
