#include "Engine/Primitives/PointLight.h"

namespace enzo {

geo::PointLight::PointLight(std::string_view path) : Primitive(path)
{
    addAttribute<Matrix4>(attr::AttributeOwner::PRIMITIVE, "transform", true);
    addAttribute<Vector3>(attr::AttributeOwner::PRIMITIVE, "color", true);
    addAttribute<floatT>(attr::AttributeOwner::PRIMITIVE, "intensity", true);
    addAttribute<floatT>(attr::AttributeOwner::PRIMITIVE, "exposure", true);
    setTransform(Matrix4::Identity());
    setColor(Vector3::Ones());
    setIntensity(1.f);
    setExposure(0.f);
}

Matrix4 geo::PointLight::getTransform() const { return getPrimitiveIntrinsic<Matrix4>("transform"); }

void geo::PointLight::setTransform(const Matrix4& xform)
{
    setPrimitiveIntrinsic<Matrix4>("transform", xform);
}

Vector3 geo::PointLight::getColor() const { return getPrimitiveIntrinsic<Vector3>("color"); }

void geo::PointLight::setColor(const Vector3& color) { setPrimitiveIntrinsic<Vector3>("color", color); }

floatT geo::PointLight::getIntensity() const { return getPrimitiveIntrinsic<floatT>("intensity"); }

void geo::PointLight::setIntensity(floatT intensity)
{
    setPrimitiveIntrinsic<floatT>("intensity", intensity);
}

floatT geo::PointLight::getExposure() const { return getPrimitiveIntrinsic<floatT>("exposure"); }

void geo::PointLight::setExposure(floatT exposure)
{
    setPrimitiveIntrinsic<floatT>("exposure", exposure);
}

void geo::PointLight::applyTransform(const Matrix4& mat, TransformClass transformClass)
{
    if (!hasFlag(transformClass, TransformClass::PRIMITIVE)) return;
    setTransform(mat * getTransform());
}

} // namespace enzo
