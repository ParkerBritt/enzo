#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <vector>

struct ParameterSerializable
{
    std::string name;
    std::vector<double> floatValues;
    std::vector<int64_t> intValues;
    std::vector<std::string> stringValues;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(floatValues),
           CEREAL_NVP(intValues),
           CEREAL_NVP(stringValues));
    }
};
