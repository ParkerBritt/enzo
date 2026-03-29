#include <string>
#include <cereal/types/string.hpp>

struct OperatorSerializable
{
    std::string typeName;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( CEREAL_NVP(typeName) );
    }
};
