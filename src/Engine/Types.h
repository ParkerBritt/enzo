#pragma once
#include <Eigen/Dense>

namespace enzo
{

    namespace ga
    {
        enum class AttributeOwner
        {
            POINT,
            VERTEX,
            PRIMITIVE,
            GLOBAL
        };
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
