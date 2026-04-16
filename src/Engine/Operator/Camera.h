#pragma once
#include "Engine/Operator/Primitive.h"

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
    Camera();
    Camera(const Camera& other);
    Camera& operator=(const Camera& rhs);
    ~Camera() override = default;

    PrimType getType() const override { return PrimType::CAMERA; }
    std::shared_ptr<Primitive> clone() const override { return std::make_shared<Camera>(*this); }
    TransformClass transformType() const override { return TransformClass::PRIMITIVE; }
    void applyTransform(const bt::Matrix4 &mat, TransformClass transformClass) override;

    bt::Matrix4 getTransform() const;
    void setTransform(const bt::Matrix4& xform);

private:
    ga::AttributeHandle<bt::Matrix4> transformHandle_;
};
}
