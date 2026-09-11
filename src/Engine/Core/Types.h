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
#include <type_traits>
#include <vector>

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

/// @brief Returns every part of the geometry that can own an attribute.
inline std::vector<AttributeOwner> getAllOwners()
{
    return {AttributeOwner::POINT,
            AttributeOwner::VERTEX,
            AttributeOwner::FACE,
            AttributeOwner::PRIMITIVE};
}

/// @brief Returns every type an attribute can store.
inline std::vector<AttributeType> getAllTypes()
{
    return {AttributeType::intT,
            AttributeType::floatT,
            AttributeType::listT,
            AttributeType::vectorT,
            AttributeType::boolT,
            AttributeType::matrixT};
}

/// @brief Returns the part of the geometry a name stands for, e.g. "point".
/// @return The owner, or empty when nothing owns attributes under that name.
inline std::optional<AttributeOwner> getOwner(const std::string& name)
{
    if (name == "point") return AttributeOwner::POINT;
    if (name == "vertex") return AttributeOwner::VERTEX;
    if (name == "face") return AttributeOwner::FACE;
    if (name == "primitive") return AttributeOwner::PRIMITIVE;
    return std::nullopt;
}

/// @brief Returns the type a name stands for, e.g. "vector".
/// @return The type, or empty when no type goes by that name.
inline std::optional<AttributeType> getType(const std::string& name)
{
    if (name == "int") return AttributeType::intT;
    if (name == "float") return AttributeType::floatT;
    if (name == "list") return AttributeType::listT;
    if (name == "vector") return AttributeType::vectorT;
    if (name == "bool") return AttributeType::boolT;
    if (name == "matrix") return AttributeType::matrixT;
    return std::nullopt;
}
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

namespace attr {
/// @brief Returns the attribute type that stores values of the given C++ type.
template <typename T> constexpr AttributeType getAttributeType()
{
    if constexpr (std::is_same_v<T, intT>) return AttributeType::intT;
    else if constexpr (std::is_same_v<T, floatT>) return AttributeType::floatT;
    else if constexpr (std::is_same_v<T, boolT>) return AttributeType::boolT;
    else if constexpr (std::is_same_v<T, Vector3>) return AttributeType::vectorT;
    else if constexpr (std::is_same_v<T, Matrix4>) return AttributeType::matrixT;
    else static_assert(sizeof(T) == 0, "No attribute type stores this C++ type");
}
} // namespace attr

namespace prm {
enum class Type
{
    STRING,
    FLOAT,
    BOOL,
    INT,
    TOGGLE,
    GROUP,
    DROPDOWN,
    RAMP,
    DIVIDER
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
    case Type::DIVIDER:
        return "divider";
    }
    return "";
}

/// @brief Every parameter type, in declaration order.
inline constexpr std::array kAllTypes = {
    Type::STRING,
    Type::FLOAT,
    Type::BOOL,
    Type::INT,
    Type::TOGGLE,
    Type::GROUP,
    Type::DROPDOWN,
    Type::RAMP,
    Type::DIVIDER
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
        return ValueType::Float;
    case Type::INT:
    case Type::BOOL:
    case Type::TOGGLE:
    // Multiparm parameters (like ramp) use integers to represent their instance
    // count and store the actual data in their instances.
    case Type::RAMP:
    // Dividers are purely visual and store an int nobody reads.
    case Type::DIVIDER:
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
