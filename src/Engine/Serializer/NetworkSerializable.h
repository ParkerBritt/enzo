#include "Engine/Network/NodeSnapshot.h"
#include "Engine/Serializer/ConnectionSerializable.h"
#include "Engine/Serializer/TimelineSerializable.h"
#include <cereal/types/vector.hpp>
#include <vector>

struct NetworkSerializable
{
    std::vector<enzo::nt::NodeSnapshot> nodes;
    std::vector<ConnectionSerializable> connections;
    TimelineSerializable timeline;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(nodes), CEREAL_NVP(connections), CEREAL_NVP(timeline));
    }
};
