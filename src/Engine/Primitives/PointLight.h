#pragma once
#include "Engine/Primitives/Primitive.h"

namespace enzo::geo {

/**
 * @class enzo::geo::PointLight
 * @brief Light that shines equally in every direction from a single position.
 *
 * @note Stores its transform, color and intensity as intrinsic primitive
 * attributes, and has no points, vertices or faces.
 */
class PointLight : public Primitive
{
  public:
    PointLight(std::string_view path = "/pointLight");
    PointLight(const PointLight& other);
    PointLight& operator=(const PointLight& rhs);
    ~PointLight() override = default;

    PrimType getType() const override { return PrimType::POINT_LIGHT; }
    std::shared_ptr<Primitive> clone() const override
    {
        return std::make_shared<PointLight>(*this);
    }
    TransformClass transformType() const override { return TransformClass::PRIMITIVE; }
    void applyTransform(const Matrix4& mat, TransformClass transformClass) override;

    Matrix4 getTransform() const;
    void setTransform(const Matrix4& xform);
    Vector3 getColor() const;
    void setColor(const Vector3& color);
    floatT getIntensity() const;
    void setIntensity(floatT intensity);

  private:
    attr::AttributeHandle<Matrix4> transformHandle_;
    attr::AttributeHandle<Vector3> colorHandle_;
    attr::AttributeHandle<floatT> intensityHandle_;
};
} // namespace enzo::geo
