#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Network/CookContext.h"
#include "Engine/Network/NodePacket.h"
#include <boost/config.hpp>
#include <string>
#include <vector>

namespace enzo::prm {
class Ramp;
}

namespace enzo::nt {
class Node;

/**
 * @brief The base class a node author subclasses to write a node's behaviour.
 *
 * One of these is built for every cook and thrown away when the cook ends, so
 * anything it stores is scoped to that cook and cannot leak into the next one.
 * Cross cook state belongs on nt::Node instead, where the engine can invalidate
 * it.
 *
 * Example
 *
 * ```
 * class Circle : public nt::NodeImpl
 * {
 *     using NodeImpl::NodeImpl;
 *     void cook() override { setOutputPacket(0, buildMesh()); }
 * };
 * ENZO_REGISTER_NODE(circle, Circle)
 * ```
 *
 * @note Every cook time query lives here as a forwarder, so an author writes
 * evalParmFloat("radius") and never names the context behind it.
 */
class BOOST_SYMBOL_EXPORT NodeImpl
{
  public:
    NodeImpl(Node& node, CookContext& context) : node_{node}, context_{context} {}
    virtual ~NodeImpl() = default;

    /// @brief Produces this node's output packets from its inputs and parameters.
    /// @post Any output left unset stays an empty packet.
    virtual void cook() = 0;

  protected:
    /// @brief Stops the cook and displays an error.
    /// @todo Add visual error to GUI
    void throwError(std::string error);

    /// @brief Displays a warning on the node without interrupting the cook.
    /// @todo Add visual warning to GUI
    void throwWarning(std::string warning);

    /// @brief Returns whether anything downstream is asking for this output.
    /// @todo Always true until nt::Node tracks dirtiness per output.
    bool outputRequested(unsigned int outputIndex);

    /// @brief Hands finished geometry to one of the node's outputs.
    /// @todo std::move the geometry instead of copying it.
    void setOutputPacket(unsigned int outputIndex, enzo::NodePacket packet);

    /// @brief Returns a writable copy of the geometry arriving at one input.
    enzo::NodePacket cloneInputPacket(unsigned int inputIndex);
    /// @brief Returns whether an input has anything connected to it.
    bool hasInput(unsigned int inputIndex);

    floatT evalParmFloat(std::string_view parmName, const unsigned int index = 0) const;
    intT evalParmInt(std::string_view parmName, const unsigned int index = 0) const;
    boolT evalParmBool(std::string_view parmName, const unsigned int index = 0) const;
    String evalParmString(std::string_view parmName, const unsigned int index = 0) const;

    // Read every value of a multi value parameter at once, where the singular
    // forms above read one value by index.
    std::vector<floatT> evalParmFloats(std::string_view parmName) const;
    std::vector<intT> evalParmInts(std::string_view parmName) const;
    std::vector<String> evalParmStrings(std::string_view parmName) const;

    Vector2 evalParmVector2(std::string_view parmName) const;
    Vector3 evalParmVector3(std::string_view parmName) const;

    /// @brief Snapshots a ramp parameter into a sampler for use during the cook.
    enzo::prm::Ramp evalParmRamp(std::string_view parmName) const;

  private:
    Node& node_;
    CookContext& context_;
};

} // namespace enzo::nt
