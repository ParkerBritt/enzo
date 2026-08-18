#include "Engine/Network/NodeSnapshot.h"
#include "Engine/Serializer/ConnectionSerializable.h"
#include <cereal/types/vector.hpp>
#include <vector>

struct NetworkSerializable
{
    std::vector<enzo::nt::NodeSnapshot> nodes;
    std::vector<ConnectionSerializable> connections;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(nodes), CEREAL_NVP(connections));
    }
};
