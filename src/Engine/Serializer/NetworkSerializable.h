#include "OperatorSerializable.h"
#include <vector>
#include <cereal/types/vector.hpp>

struct NetworkSerializable
{
    std::vector<OperatorSerializable> nodes;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( CEREAL_NVP(nodes) );
    }
};
