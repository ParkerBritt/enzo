#include "Engine/Primitives/Camera.h"

namespace enzo {


geo::Camera::Camera(std::string_view path)
    : transformHandle_{addMatrix4Attribute(attr::AttributeOwner::PRIMITIVE, "transform", true)},
    Primitive(path)
{
    transformHandle_.addValue(Matrix4::Identity());
}

geo::Camera::Camera(const Camera& other)
    : Primitive(other),
      transformHandle_{attr::AttributeHandle<Matrix4>(getAttribByName(attr::AttributeOwner::PRIMITIVE, "transform", true))}
{
}

geo::Camera& geo::Camera::operator=(const Camera& rhs)
{
    if (this == &rhs) return *this;
    Primitive::operator=(rhs);
    transformHandle_ = attr::AttributeHandle<Matrix4>(getAttribByName(attr::AttributeOwner::PRIMITIVE, "transform", true));
    return *this;
}

Matrix4 geo::Camera::getTransform() const
{
    return transformHandle_.getValue(0);
}

void geo::Camera::setTransform(const Matrix4& xform)
{
    transformHandle_.setValue(0, xform);
}

void geo::Camera::applyTransform(const Matrix4 &mat, TransformClass transformClass) {
    if ((transformClass & TransformClass::PRIMITIVE) == TransformClass::NONE) return;
    setTransform(mat * getTransform());
}

} // namespace enzo
