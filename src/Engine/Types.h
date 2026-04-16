/**
 * @file Types.h
 * @brief Basic attribute, parameter, and node types for Enzo.
 */

#pragma once
#include <Eigen/Dense>
#include <cstdint>

namespace enzo {

namespace ga {
/**
 * @brief The segment of geometry that owns a particular attribute
 *
 * - POINT attributes are stored per point, these attributes have an value for each point.
 * - VERTEX attributes are stored per vertex, these attributes have an value for each vertex.
 * - FACE attributes are stored per face, these attributes have an value for each face.
 * - PRIMITIVE attributes are stored per primitive object, these attributes only have one value.
 */
enum class AttributeOwner { POINT, VERTEX, FACE, PRIMITIVE };
/**
 * @brief Data types available to store attribute values in.
 */
enum class AttributeType {
    intT,
    floatT,
    listT,
    vectorT,
    boolT,
    matrixT,
};
using AttrType = AttributeType;
using AttrOwner = AttributeOwner;
/**
 * @brief ga::Offset is the index of an element in a given AttributeOwner.
 *
 * Eg. point index, vertex index, primitive index, or global index.
 * This different but similar in concept to a point number. This
 * value will stay consistant through geometry modification such
 * as adding and deleting points unlress defragmented.
 */
using Offset = size_t;
} // namespace ga
namespace geo {
enum class PrimType { MESH, CAMERA };
}

enum class TransformClass : uint8_t {
    NONE = 0,
    POINT = 1,
    PRIMITIVE = 2,
    POINT_PRIORITY = 3 // POINT | PRIMITIVE
};

// Enum class doesn't support bitwise ops natively. These allow bit-testing
// e.g. (transformClass & TransformClass::POINT) != TransformClass::NONE
inline TransformClass operator|(TransformClass a, TransformClass b) {
    return static_cast<TransformClass>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline TransformClass operator&(TransformClass a, TransformClass b) {
    return static_cast<TransformClass>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
// e.g. hasFlag(prim.transformType(), TransformClass::POINT)
inline bool hasFlag(TransformClass value, TransformClass flag) {
    return (value & flag) != TransformClass::NONE;
}

// Basic types
namespace bt {
using floatT = double;
using intT = int64_t;
using boolT = bool;
using Vector2f = Eigen::Vector2f;
using Vector3 = Eigen::Vector3d;
using Vector4 = Eigen::Vector4d;
using Matrix4 = Eigen::Matrix4d;
using String = std::string;
} // namespace bt
namespace prm {
enum class Type { LIST_TERMINATOR, STRING, FLOAT, BOOL, XYZ, INT, TOGGLE };
}
namespace nt {
/**
 * @brief The unique ID assigned to each node in the network.
 */
using OpId = uint64_t;

enum class SocketIOType { Input, Output };
} // namespace nt
} // namespace enzo
