#include "Engine/Serializer/ParameterSerializable.h"
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <vector>

struct OperatorSerializable
{
    std::string typeName;
    std::string path;
    float posX = 0;
    float posY = 0;
    std::vector<ParameterSerializable> parameters;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(typeName),
           CEREAL_NVP(path),
           CEREAL_NVP(posX),
           CEREAL_NVP(posY),
           CEREAL_NVP(parameters));
    }
};
