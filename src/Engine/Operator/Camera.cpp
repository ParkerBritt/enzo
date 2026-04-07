#include "Engine/Operator/Camera.h"

using namespace enzo;

geo::Camera::Camera()
    : posHandle_{addVector3Attribute(ga::AttributeOwner::PRIMITIVE, "P", true)}
{
    posHandle_.addValue(bt::Vector3(0.0, 0.0, 0.0));
    path_ = "/camera";
}

geo::Camera::Camera(const Camera& other)
    : Primitive(other),
      posHandle_{ga::AttributeHandle<bt::Vector3>(getAttribByName(ga::AttributeOwner::PRIMITIVE, "P", true))}
{
}

geo::Camera& geo::Camera::operator=(const Camera& rhs)
{
    if (this == &rhs) return *this;
    Primitive::operator=(rhs);
    posHandle_ = ga::AttributeHandle<bt::Vector3>(getAttribByName(ga::AttributeOwner::PRIMITIVE, "P", true));
    return *this;
}

bt::Vector3 geo::Camera::getPosition() const
{
    return posHandle_.getValue(0);
}

void geo::Camera::setPosition(const bt::Vector3& pos)
{
    posHandle_.setValue(0, pos);
}
