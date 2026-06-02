#pragma once

#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"

namespace enzo {
class Transform {
  public:
    Transform(attr::Attribute &attribute, attr::Offset offset) {
        // Compute 4x4 transform matrix from attributes

        // Use vector attribute as the position in the matrix
        if (attribute.getType() == attr::AttrType::vectorT) {
            mat_ = bt::Matrix4::Identity();
            mat_.col(3).head<3>() = attribute.getVector3(offset);

            // Use matrix types directly
        } else if (attribute.getType() == attr::AttrType::matrixT) {
            mat_ = attribute.getMatrix4(offset);

            // Fallback to identity
        } else {
            mat_ = bt::Matrix4::Identity();
        }
    }

    const bt::Matrix4 &getMatrix() const { return mat_; }

  private:
    bt::Matrix4 mat_;
};
} // namespace enzo
