#include "OperatorSerializable.h"
#include "ConnectionSerializable.h"
#include <vector>
#include <cereal/types/vector.hpp>

struct NetworkSerializable
{
    std::vector<OperatorSerializable> nodes;
    std::vector<ConnectionSerializable> connections;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( CEREAL_NVP(nodes), CEREAL_NVP(connections) );
    }
};
