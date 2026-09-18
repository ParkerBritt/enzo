#include "Engine/Core/Types.h"
#include <cereal/cereal.hpp>

struct TimelineSerializable
{
    enzo::intT startFrame = 1;
    enzo::intT endFrame = 240;
    enzo::floatT fps = 24;
    enzo::floatT frame = 1;

    template <class Archive> void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(startFrame), CEREAL_NVP(endFrame), CEREAL_NVP(fps), CEREAL_NVP(frame));
    }
};
