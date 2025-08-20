/**
 * @file Types.h
 * @brief Basic attribute, parameter, and node types for Enzo.
 */

#pragma once
#include <Eigen/Dense>

namespace enzo
{

    namespace ga
    {
        /**
         * @brief The segment of geometry that owns a particular attribute
         *
         * - POINT attributes are stored per point, these attributes have an value for each point.
         * - VERTEX attributes are stored per vertex, these attributes have an value for each vertex.
         * - PRIMITIVE attributes are stored per primitive, these attributes have an value for each primitive.
         * - GLOBAL attributes are stored globaly, these attributes only have one value.
         */
        enum class AttributeOwner
        {
            POINT,
            VERTEX,
            PRIMITIVE,
            GLOBAL
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
    }
    // basic types types
    namespace bt
    {
        using floatT = double;
        using intT = int64_t;
        using boolT = bool;
        using Vector3 = Eigen::Vector3d;
        using Vector4 = Eigen::Vector4d;
        using String = std::string;
    }
    namespace prm
    {
        enum class Type
        {
            LIST_TERMINATOR,
            STRING,
            FLOAT,
            BOOL,
            XYZ,
            INT,
            TOGGLE
        };
    }
    namespace nt
    {
        using OpId = uint64_t;

        enum class SocketIOType {
            Input,
            Output
        };
    }
}
