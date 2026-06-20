#pragma once

#include "Engine/Attribute/Attribute.h"
#include "Engine/Core/Types.h"

namespace enzo {
class Point
{
  public:
    Point(attr::attribVector& attribute, Offset offset) : attributes_(attribute), offset_(offset) {}

    Vector3 getPosition() { return Vector3(0, 0, 0); };
    Matrix4 getTransform() { return Matrix4(); };

  private:
    attr::attribVector& attributes_;
    Offset offset_;
};
} // namespace enzo
