#pragma once

#include "Engine/Core/Types.h"
#include <Eigen/Geometry>
#include <numbers>

namespace enzo::attr {
class Attribute;
}

namespace enzo {

/**
 * @class enzo::Transform
 * @brief An affine transform that composes translation, rotation, and scale.
 *
 * Builder methods mutate in place and return a reference so calls can be
 * chained. Factory methods construct a Transform from other representations
 * such as an attribute value or an orientation frame.
 *
 * @note The transform is stored as an Eigen affine transform whose operations
 *       skip the homogeneous bottom row, so composing and applying it costs
 *       less than a full 4x4 multiply. Builders and accessors are inline so
 *       they survive per element hot paths.
 */
class Transform
{
  public:
    using Affine = Eigen::Transform<floatT, 3, Eigen::Affine>;

    /** @brief Constructs an identity transform. */
    Transform() : xform_(Affine::Identity()) {}

    /** @brief Wraps an existing matrix. */
    explicit Transform(const Matrix4& matrix) : xform_(matrix) {}

    // Factories

    /**
     * @brief Reads a transform from a single attribute value.
     * @return The transform encoded at the given offset.
     *
     * @note A vector attribute becomes a translation. A matrix attribute is
     *       used directly. Any other type yields the identity.
     */
    static Transform fromAttribute(const attr::Attribute& attribute, Offset offset);

    /**
     * @brief Builds an orientation frame that points down a forward axis.
     * @return A transform whose rotation aligns to the frame, placed at translation.
     *
     * @note The rotation maps the local Z axis onto forward and the local Y
     *       axis toward up. The up vector resolves the remaining roll. When
     *       forward and up are parallel the rotation falls back to identity.
     */
    static Transform lookAt(const Vector3& translation, const Vector3& forward, const Vector3& up);

    // Builders compose in the current local frame, so the first call is applied
    // to a point first and later calls wrap it.

    /** @brief Moves the local origin by an offset. */
    Transform& translate(const Vector3& offset)
    {
        xform_.translate(offset);
        return *this;
    }

    /** @brief Rotates in X, then Y, then Z, with angles given in degrees. */
    Transform& rotateEuler(const Vector3& degrees)
    {
        const Vector3 radians = degrees * (std::numbers::pi / 180.0);
        xform_.rotate(
            Eigen::AngleAxis<floatT>(radians.z(), Vector3::UnitZ()) *
            Eigen::AngleAxis<floatT>(radians.y(), Vector3::UnitY()) *
            Eigen::AngleAxis<floatT>(radians.x(), Vector3::UnitX())
        );
        return *this;
    }

    /** @brief Scales each local axis independently. */
    Transform& scale(const Vector3& factors)
    {
        xform_.scale(factors);
        return *this;
    }

    /** @brief Scales all local axes by the same factor. */
    Transform& scale(floatT uniform)
    {
        xform_.scale(uniform);
        return *this;
    }

    /** @brief Composes another transform into this one in the local frame. */
    Transform& compose(const Transform& other)
    {
        xform_ = xform_ * other.xform_;
        return *this;
    }

    const Matrix4& getMatrix() const { return xform_.matrix(); }

  private:
    Affine xform_;
};

} // namespace enzo
