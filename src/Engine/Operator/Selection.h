#pragma once
#include <string>
#include "Engine/Operator/NodePacket.h"

namespace enzo
{
    class Selection
    {
        Selection(NodePacket, std::string);

        void getPoints();
        void getFaces();
        void getVertices();
        void getPrimitives();
        bool containsFullPrimitive();
    };
}
