#include "Engine/Attribute/Transform.h"
#include "Engine/Attribute/Attribute.h"

namespace enzo {

TransformOrder getTransformOrder(std::string_view name)
{
    if (name == "str") return TransformOrder::STR;
    if (name == "rst") return TransformOrder::RST;
    if (name == "rts") return TransformOrder::RTS;
    if (name == "tsr") return TransformOrder::TSR;
    if (name == "trs") return TransformOrder::TRS;
    return TransformOrder::SRT;
}

Transform Transform::fromAttribute(const attr::Attribute& attribute, Offset offset)
{
    // Vector attribute becomes a translation.
    if (attribute.getType() == attr::AttrType::vectorT)
    {
        return Transform().translate(attribute.getVector3(offset));
    }

    // Matrix attribute is used directly.
    if (attribute.getType() == attr::AttrType::matrixT)
    {
        return Transform(attribute.getMatrix4(offset));
    }

    return Transform();
}

Transform Transform::fromComponents(
    const Vector3& translation,
    const Vector3& rotationDegrees,
    const Vector3& scale,
    TransformOrder order
)
{
    Transform transform;

    // Composes each case backwards, since every builder wraps the ones before it.
    switch (order)
    {
    case TransformOrder::SRT:
        transform.translate(translation).rotateEuler(rotationDegrees).scale(scale);
        break;
    case TransformOrder::STR:
        transform.rotateEuler(rotationDegrees).translate(translation).scale(scale);
        break;
    case TransformOrder::RST:
        transform.translate(translation).scale(scale).rotateEuler(rotationDegrees);
        break;
    case TransformOrder::RTS:
        transform.scale(scale).translate(translation).rotateEuler(rotationDegrees);
        break;
    case TransformOrder::TSR:
        transform.rotateEuler(rotationDegrees).scale(scale).translate(translation);
        break;
    case TransformOrder::TRS:
        transform.scale(scale).rotateEuler(rotationDegrees).translate(translation);
        break;
    }

    return transform;
}

Transform Transform::lookAt(const Vector3& translation, const Vector3& forward, const Vector3& up)
{
    Transform result;
    result.xform_.translation() = translation;

    const Vector3 zAxis = forward.normalized();
    Vector3 xAxis = up.cross(zAxis);
    const floatT xLength = xAxis.norm();

    // Forward and up are parallel, so the roll is undefined and the rotation
    // stays identity.
    if (xLength < 1e-6) return result;

    xAxis /= xLength;
    const Vector3 yAxis = zAxis.cross(xAxis);

    Eigen::Matrix<floatT, 3, 3> rotation;
    rotation.col(0) = xAxis;
    rotation.col(1) = yAxis;
    rotation.col(2) = zAxis;
    result.xform_.linear() = rotation;

    return result;
}

} // namespace enzo
