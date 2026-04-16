#include "ParameterSerializable.h"
#include <string>
#include <vector>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

struct OperatorSerializable
{
    std::string typeName;
    float posX = 0;
    float posY = 0;
    std::vector<ParameterSerializable> parameters;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( CEREAL_NVP(typeName), CEREAL_NVP(posX), CEREAL_NVP(posY), CEREAL_NVP(parameters) );
    }
};
