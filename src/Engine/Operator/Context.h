#pragma once

#include "Engine/Types.h"
#include "Engine/Operator/Geometry.h"

namespace enzo::nt
{
    class NetworkManager;
}

namespace enzo::op
{
/**
* @class Context
* @brief Provides network context for the cookOp function.
*
* The context class is an interface for the cookOp function that provides important runtime context about the network. It allows querying parameters, reading input geometry, and in the future provides values like time.
*/
class Context
{
public:
    Context(enzo::nt::OpId opId, enzo::nt::NetworkManager& networkManager);
    enzo::geo::Geometry cloneInputGeo(unsigned int inputIndex);
    bt::floatT evalFloatParm(const char* parmName, const unsigned int index=0) const;
    bt::intT evalIntParm(const char* parmName, const unsigned int index=0) const;
    bt::boolT evalBoolParm(const char* parmName, const unsigned int index=0) const;
    bt::String evalStringParm(const char* parmName, const unsigned int index=0) const;
private:
    enzo::nt::OpId opId_;
    enzo::nt::NetworkManager& networkManager_;

};
}
