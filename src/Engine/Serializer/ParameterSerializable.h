#include "Engine/Core/Types.h"
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <vector>

struct ParameterSerializable
{
    std::string name;
    std::vector<enzo::floatT> floatValues;
    std::vector<enzo::intT> intValues;
    std::vector<std::string> stringValues;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(floatValues),
           CEREAL_NVP(intValues),
           CEREAL_NVP(stringValues));
    }
};
