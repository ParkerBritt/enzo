#pragma once
#include <boost/config.hpp>
#include <filesystem>

namespace enzo::nt {

/**
 * @brief The startup pass that fills the node type table from the node folders on disk.
 *
 * Every node is a folder holding a node.yaml, and the builtin nodes load
 * through the same path an installed one does. Every folder is read up front so
 * the tab menu has every node's label the moment it opens, while a node's
 * shared library only opens once a node from it is loaded.
 *
 * @note Nodes whose implementation cannot be resolved are reported and skipped.
 */
class BOOST_SYMBOL_EXPORT NodeLoader
{
  public:
    /// @brief Reads every node folder and registers what it finds.
    /// @note Does nothing when called again.
    static void loadNodes();

    /// @brief Returns the directory the node folders live in.
    /// @note Throws std::runtime_error when no candidate directory exists.
    static std::filesystem::path getNodesDirectory();
};

} // namespace enzo::nt
