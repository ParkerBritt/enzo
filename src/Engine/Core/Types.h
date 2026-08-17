/**
 * @file Types.h
 * @brief Basic attribute, parameter, and node types for Enzo.
 */

#pragma once
#include <Eigen/Dense>
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace enzo {

namespace attr {
/**
 * @brief The segment of geometry that owns a particular attribute
 *
 * - POINT attributes are stored per point, these attributes have an value for each point.
 * - VERTEX attributes are stored per vertex, these attributes have an value for each vertex.
 * - FACE attributes are stored per face, these attributes have an value for each face.
 * - PRIMITIVE attributes are stored per primitive object, these attributes only have one value.
 */
enum class AttributeOwner
{
    POINT,
    VERTEX,
    FACE,
    PRIMITIVE
};
/**
 * @brief Data types available to store attribute values in.
 */
enum class AttributeType
{
    intT,
    floatT,
    listT,
    vectorT,
    boolT,
    matrixT,
};
using AttrType = AttributeType;
using AttrOwner = AttributeOwner;
} // namespace attr
namespace geo {
enum class PrimType
{
    MESH,
    CAMERA
};
}

enum class TransformClass : uint8_t
{
    NONE = 0,
    POINT = 1,
    PRIMITIVE = 2,
    POINT_PRIORITY = 3 // POINT | PRIMITIVE
};

// Enum class doesn't support bitwise ops natively. These allow bit-testing
// e.g. (transformClass & TransformClass::POINT) != TransformClass::NONE
inline TransformClass operator|(TransformClass a, TransformClass b)
{
    return static_cast<TransformClass>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline TransformClass operator&(TransformClass a, TransformClass b)
{
    return static_cast<TransformClass>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
// e.g. hasFlag(prim.transformType(), TransformClass::POINT)
inline bool hasFlag(TransformClass value, TransformClass flag)
{
    return (value & flag) != TransformClass::NONE;
}

// Basic types
using floatT = float;
using intT = int64_t;
using boolT = bool;
using Vector2 = Eigen::Vector2f;
using Vector3 = Eigen::Vector3f;
using Vector4 = Eigen::Vector4f;
using Matrix3 = Eigen::Matrix3f;
using Matrix4 = Eigen::Matrix4f;
using String = std::string;
/**
 * @brief enzo::Index is the continuous index of an element in a given AttributeOwner.
 *
 * Eg. point index, vertex index, primitive index, or global index.
 * This is usually provided by the user where enzo::Offset is used internally.
 */
using Index = size_t;
/**
 * @brief enzo::Offset is the internal discontinuous index of an element in a given AttributeOwner.
 *
 * Eg. point offset, vertex offset, primitive offset, or global offset.
 * This different but similar in concept to the index. This
 * value will stay consistant through geometry modification such
 * as adding and deleting points unless defragmented.
 */
using Offset = size_t;
namespace prm {
enum class Type
{
    STRING,
    FLOAT,
    BOOL,
    XYZ,
    INT,
    TOGGLE,
    GROUP,
    DROPDOWN,
    RAMP,
    SPACER
};
enum class Direction
{
    HORIZONTAL,
    VERTICAL
};
/**
 * @brief Which kind of value a parameter stores.
 *
 * Every prm::Type maps to one of these through toValueType, and the rest of the
 * value handling switches on this tag.
 */
enum class ValueType
{
    Float,
    Int,
    String
};

/// @brief The lowercase canonical name of a parameter type, e.g. "dropdown".
inline std::string toString(Type type)
{
    // Using a switch to catch missing cases at compile time.
    switch (type)
    {
    case Type::STRING:
        return "string";
    case Type::FLOAT:
        return "float";
    case Type::BOOL:
        return "bool";
    case Type::XYZ:
        return "xyz";
    case Type::INT:
        return "int";
    case Type::TOGGLE:
        return "toggle";
    case Type::GROUP:
        return "group";
    case Type::DROPDOWN:
        return "dropdown";
    case Type::RAMP:
        return "ramp";
    case Type::SPACER:
        return "spacer";
    }
    return "";
}

/// @brief Every parameter type, in declaration order.
inline constexpr std::array kAllTypes = {
    Type::STRING,
    Type::FLOAT,
    Type::BOOL,
    Type::XYZ,
    Type::INT,
    Type::TOGGLE,
    Type::GROUP,
    Type::DROPDOWN,
    Type::RAMP,
    Type::SPACER
};

/// @brief Returns the parameter type a canonical name stands for, e.g. "dropdown".
/// @return The type, or nullopt when no type carries that name.
inline std::optional<Type> toType(const std::string& name)
{
    for (Type type : kAllTypes)
        if (toString(type) == name) return type;
    return std::nullopt;
}

/// @brief Returns the kind of value a parameter of this type stores.
inline ValueType toValueType(Type type)
{
    switch (type)
    {
    case Type::FLOAT:
    case Type::XYZ:
        return ValueType::Float;
    case Type::INT:
    case Type::BOOL:
    case Type::TOGGLE:
    // Multiparm parameters (like ramp) use integers to represent their instance
    // count and store the actual data in their instances.
    case Type::RAMP:
    // Spacers are purely visual and store an int nobody reads.
    case Type::SPACER:
        return ValueType::Int;
    case Type::STRING:
    case Type::DROPDOWN:
        return ValueType::String;
    case Type::GROUP:
        return ValueType::Float;
    }
    return ValueType::Float;
}
} // namespace prm
namespace nt {
/**
 * @brief The unique ID assigned to each node in the network.
 */
using NodeId = uint64_t;

/// @brief The id that names no node, since real ids start at 1.
constexpr NodeId nullNode = 0;

enum class SocketIOType
{
    Input,
    Output
};
} // namespace nt
} // namespace enzo
