#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Parameter.h"
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
    // Multiparm parameters such as ramps store nothing flat. Each instance is a
    // list of field models, so the structure nests one level per instance.
    std::vector<std::vector<ParameterSerializable>> instances;

    bool operator==(const ParameterSerializable& other) const = default;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name),
           CEREAL_NVP(floatValues),
           CEREAL_NVP(intValues),
           CEREAL_NVP(stringValues),
           CEREAL_NVP(instances));
    }
};

// Capture a parameter's live state into a serializable model. Multiparms recurse
// one model per instance field, flat parameters store their value vector.
ParameterSerializable toSerializable(enzo::prm::Parameter& parameter);

// Snapshots a flat parameter from its name and values without a live Parameter.
ParameterSerializable toSerializable(std::string name, const enzo::prm::PrmValues& values);

// Write a serialized model back onto a live parameter. Multiparms reconcile their
// instance count to the model before writing each field by name.
void applySerializable(enzo::prm::Parameter& parameter, const ParameterSerializable& model);
