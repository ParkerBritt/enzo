#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NodePacket.h"

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
class Context
{
  public:
    Context(enzo::nt::OpId opId, enzo::nt::NetworkManager& networkManager);
    enzo::NodePacket cloneInputPacket(unsigned int inputIndex);
    bool hasInput(unsigned int inputIndex);
    floatT evalParmFloat(std::string_view parmName, const unsigned int index = 0) const;
    intT evalParmInt(std::string_view parmName, const unsigned int index = 0) const;
    boolT evalParmBool(std::string_view parmName, const unsigned int index = 0) const;
    String evalParmString(std::string_view parmName, const unsigned int index = 0) const;

    // Read every value of a parameter as a list. The singular forms above read
    // one value by index, these read them all at once.
    std::vector<floatT> evalParmFloats(std::string_view parmName) const;
    std::vector<intT> evalParmInts(std::string_view parmName) const;
    std::vector<String> evalParmStrings(std::string_view parmName) const;

    // Read the first two float components of a parameter as a Vector2. Missing
    // components default to zero.
    Vector2 evalParmVector2(std::string_view parmName) const;

    // Read the first three float components of a parameter as a Vector3. Missing
    // components default to zero.
    Vector3 evalParmVector3(std::string_view parmName) const;

  private:
    enzo::nt::OpId opId_;
    enzo::nt::NetworkManager& networkManager_;
};
} // namespace enzo::op
