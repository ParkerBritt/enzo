#include <string>
#include <cereal/types/string.hpp>

struct OperatorSerializable
{
    std::string typeName;
    float posX = 0;
    float posY = 0;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( CEREAL_NVP(typeName), CEREAL_NVP(posX), CEREAL_NVP(posY) );
    }
};
