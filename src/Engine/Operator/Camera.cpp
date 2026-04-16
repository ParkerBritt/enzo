#include "Engine/Operator/Camera.h"

using namespace enzo;

geo::Camera::Camera()
    : transformHandle_{addMatrix4Attribute(ga::AttributeOwner::PRIMITIVE, "transform", true)}
{
    transformHandle_.addValue(bt::Matrix4::Identity());
    path_ = "/camera";
}

geo::Camera::Camera(const Camera& other)
    : Primitive(other),
      transformHandle_{ga::AttributeHandle<bt::Matrix4>(getAttribByName(ga::AttributeOwner::PRIMITIVE, "transform", true))}
{
}

geo::Camera& geo::Camera::operator=(const Camera& rhs)
{
    if (this == &rhs) return *this;
    Primitive::operator=(rhs);
    transformHandle_ = ga::AttributeHandle<bt::Matrix4>(getAttribByName(ga::AttributeOwner::PRIMITIVE, "transform", true));
    return *this;
}

bt::Matrix4 geo::Camera::getTransform() const
{
    return transformHandle_.getValue(0);
}

void geo::Camera::setTransform(const bt::Matrix4& xform)
{
    transformHandle_.setValue(0, xform);
}

void geo::Camera::applyTransform(const bt::Matrix4 &mat, TransformClass transformClass) {
    if ((transformClass & TransformClass::PRIMITIVE) == TransformClass::NONE) return;
    setTransform(mat * getTransform());
}
