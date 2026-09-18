#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NodePacket.h"

namespace enzo::nt {
class NetworkManager;
}

namespace enzo::prm {
class Ramp;
}

namespace enzo::nt {
/**
 * @class CookContext
 * @brief The handle a node holds while it cooks to get context about the network.
 *
 * Carries the runtime context a cook reads from the network, covering parameter values, input
 * geometry and the scene time.
 */
class CookContext
{
  public:
    CookContext(enzo::nt::NodeId nodeId, enzo::nt::NetworkManager& networkManager);
    enzo::NodePacket cloneInputPacket(unsigned int inputIndex);
    bool hasInput(unsigned int inputIndex);
    unsigned int getInputCount();

    /// @brief Returns the frame the scene sits on.
    /// @note The frame can be fractional.
    floatT getFrame() const;

    /// @brief Returns the scene time in seconds, measured from the start of frame 1.
    floatT getTime() const;

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

    // Snapshot a ramp parameter into a sampler for use during cook.
    enzo::prm::Ramp evalParmRamp(std::string_view parmName) const;

  private:
    /// @brief Records that the cooking node reads the scene time.
    void recordTimeDependency_() const;

    enzo::nt::NodeId nodeId_;
    enzo::nt::NetworkManager& networkManager_;
};
} // namespace enzo::nt
