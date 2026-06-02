#pragma once
#include "Engine/Primitives/Primitive.h"

namespace enzo::geo
{

/**
* @class enzo::geo::Camera
* @brief Camera primitive with only primitive-level attributes.
*
* Unlike Mesh, Camera has no points, vertices, or faces.
* It stores a 4x4 transform matrix as a primitive attribute.
*/
class Camera : public Primitive
{
public:
    Camera(std::string_view path="/camera");
    Camera(const Camera& other);
    Camera& operator=(const Camera& rhs);
    ~Camera() override = default;

    PrimType getType() const override { return PrimType::CAMERA; }
    std::shared_ptr<Primitive> clone() const override { return std::make_shared<Camera>(*this); }
    TransformClass transformType() const override { return TransformClass::PRIMITIVE; }
    void applyTransform(const Matrix4 &mat, TransformClass transformClass) override;

    Matrix4 getTransform() const;
    void setTransform(const Matrix4& xform);

private:
    attr::AttributeHandle<Matrix4> transformHandle_;
};
}
