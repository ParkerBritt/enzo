#pragma once

#include "Engine/Network/NodeType.h"
#include <boost/config.hpp>
#include <deque>
#include <string>

namespace enzo::nt {

/**
 * @brief The sole owner of every node type the application knows about.
 *
 * The nt::NodeLoader fills the table at startup and nodes hold references into
 * it, so a type exists once no matter how many nodes are built from it. Entries
 * are never removed, which is what keeps those references valid.
 */
class BOOST_SYMBOL_EXPORT NodeTypeTable
{
  public:
    /// @brief Takes ownership of a node type and returns the stored copy.
    static const NodeType& addNodeType(NodeType nodeType);

    /// @brief Returns the type registered under a full name, such as "enzo::grid".
    /// @return The type, or nullptr when nothing carries that name.
    static const NodeType* getNodeType(const std::string& fullName);

    /// @brief Returns the type registered under a full name, for callers that cannot go on
    /// without it.
    /// @note Throws std::runtime_error when nothing carries that name.
    static const NodeType& requireNodeType(const std::string& fullName);

    /// @brief Returns every registered type, in the order they were added.
    static const std::deque<NodeType>& getData();

  private:
    // A deque rather than a vector so growing the table never moves the types
    // nodes hold references to.
    static std::deque<NodeType> nodeTypeStore_;
};

} // namespace enzo::nt
