#pragma once
#include "Engine/Operator/Primitive.h"

namespace enzo::geo
{

/**
* @class enzo::geo::Camera
* @brief Camera primitive with only primitive-level attributes.
*
* Unlike Mesh, Camera has no points, vertices, or faces.
* It stores a position as a primitive attribute.
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

    bt::Vector3 getPosition() const;
    void setPosition(const bt::Vector3& pos);

private:
    ga::AttributeHandle<bt::Vector3> posHandle_;
};
}
