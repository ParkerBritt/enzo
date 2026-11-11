#include "Engine/Primitives/Camera.h"

namespace enzo {

geo::Camera::Camera(std::string_view path) : Primitive(path)
{
    addAttribute<Matrix4>(attr::AttributeOwner::PRIMITIVE, "transform", true);
    setTransform(Matrix4::Identity());
}

Matrix4 geo::Camera::getTransform() const { return getPrimitiveIntrinsic<Matrix4>("transform"); }

void geo::Camera::setTransform(const Matrix4& xform)
{
    setPrimitiveIntrinsic<Matrix4>("transform", xform);
}

void geo::Camera::applyTransform(const Matrix4& mat, TransformClass transformClass)
{
    if (!hasFlag(transformClass, TransformClass::PRIMITIVE)) return;
    setTransform(mat * getTransform());
}

} // namespace enzo
