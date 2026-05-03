#pragma once

#include "Engine/Operator/NodePacket.h"
#include "Engine/Types.h"

namespace enzo::nt {
class NetworkManager;
}

namespace enzo::op {
/**
 * @class Context
 * @brief Provides network context for the cookOp function.
 *
 * The context class is an interface for the cookOp function that provides important runtime context
 * about the network. It allows querying parameters, reading input geometry, and in the future
 * provides values like time.
 */
class Context {
  public:
    Context(enzo::nt::OpId opId, enzo::nt::NetworkManager &networkManager);
    enzo::NodePacket cloneInputPacket(unsigned int inputIndex);
    bool hasInput(unsigned int inputIndex);
    bt::floatT evalFloatParm(std::string_view parmName, const unsigned int index = 0) const;
    bt::intT evalIntParm(std::string_view parmName, const unsigned int index = 0) const;
    bt::boolT evalBoolParm(std::string_view parmName, const unsigned int index = 0) const;
    bt::String evalStringParm(std::string_view parmName, const unsigned int index = 0) const;

  private:
    enzo::nt::OpId opId_;
    enzo::nt::NetworkManager &networkManager_;
};
} // namespace enzo::op
