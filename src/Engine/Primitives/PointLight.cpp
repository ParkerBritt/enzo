#include "Engine/Primitives/PointLight.h"

namespace enzo {

namespace {

/// @brief Returns a handle to the intrinsic primitive attribute of this name.
template <typename T> attr::AttributeHandle<T> getIntrinsic(geo::Primitive& prim, std::string_view name)
{
    return attr::AttributeHandle<T>(prim.getAttribByName(attr::AttributeOwner::PRIMITIVE, name, true));
}

} // namespace

geo::PointLight::PointLight(std::string_view path)
    : Primitive(path),
      transformHandle_{addAttribute<Matrix4>(attr::AttributeOwner::PRIMITIVE, "transform", true)},
      colorHandle_{addAttribute<Vector3>(attr::AttributeOwner::PRIMITIVE, "color", true)},
      intensityHandle_{addAttribute<floatT>(attr::AttributeOwner::PRIMITIVE, "intensity", true)}
{
    setTransform(Matrix4::Identity());
    setColor(Vector3::Ones());
    setIntensity(1.f);
}

geo::PointLight::PointLight(const PointLight& other)
    : Primitive(other), transformHandle_{getIntrinsic<Matrix4>(*this, "transform")},
      colorHandle_{getIntrinsic<Vector3>(*this, "color")},
      intensityHandle_{getIntrinsic<floatT>(*this, "intensity")}
{
}

geo::PointLight& geo::PointLight::operator=(const PointLight& rhs)
{
    if (this == &rhs) return *this;
    Primitive::operator=(rhs);
    transformHandle_ = getIntrinsic<Matrix4>(*this, "transform");
    colorHandle_ = getIntrinsic<Vector3>(*this, "color");
    intensityHandle_ = getIntrinsic<floatT>(*this, "intensity");
    return *this;
}

Matrix4 geo::PointLight::getTransform() const { return transformHandle_.getValue(0); }

void geo::PointLight::setTransform(const Matrix4& xform) { transformHandle_.setValue(0, xform); }

Vector3 geo::PointLight::getColor() const { return colorHandle_.getValue(0); }

void geo::PointLight::setColor(const Vector3& color) { colorHandle_.setValue(0, color); }

floatT geo::PointLight::getIntensity() const { return intensityHandle_.getValue(0); }

void geo::PointLight::setIntensity(floatT intensity) { intensityHandle_.setValue(0, intensity); }

void geo::PointLight::applyTransform(const Matrix4& mat, TransformClass transformClass)
{
    if (!hasFlag(transformClass, TransformClass::PRIMITIVE)) return;
    setTransform(mat * getTransform());
}

} // namespace enzo
