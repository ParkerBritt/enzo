#pragma once
#include "Engine/Network/NodeType.h"
#include <filesystem>
#include <string>
#include <vector>

namespace enzo::nt {

/**
 * @brief Where a node's behaviour comes from.
 *
 * The manifest only names the pieces. Finding the library on disk and pulling
 * the constructor out of it happens later, so a manifest still parses when the
 * binary it points at is missing.
 */
struct NodeImplementation
{
    std::string kind;
    std::string library;
    std::string constructor;
};

/**
 * @brief The in memory form of one node's manifest file.
 *
 * A node folder holds a node.yaml declaring the label, tags, input and output
 * counts, parameters, and implementation of the node. This class is that file
 * read into memory.
 *
 * Example
 *
 * ```
 * const NodeManifest manifest = NodeManifest::loadFromFile("nodes/sweep/node.yaml");
 * manifest.getNodeType().getLabel();          // "Sweep"
 * manifest.getImplementation().constructor;   // "sweep"
 * ```
 *
 * @note Nothing here touches the surrounding folder, which is what lets tests
 * build a node from a string. Scanning directories and resolving the icon and
 * the library against them is the loader's job.
 */
class NodeManifest
{
  public:
    /// @brief Returns the manifest described by a yaml document.
    /// @note Throws std::runtime_error when the document is malformed.
    static NodeManifest loadFromString(const std::string& yaml);

    /// @brief Returns the manifest described by a node.yaml file.
    /// @note Throws std::runtime_error when the file is missing or malformed.
    static NodeManifest loadFromFile(const std::filesystem::path& path);

    /// @brief Returns the node type this manifest describes, with no constructor filled in yet.
    const NodeType& getNodeType() const { return nodeType_; }
    const NodeImplementation& getImplementation() const { return implementation_; }

    /// @brief Returns the words the tab menu searches on, such as "curve" or "primitive".
    const std::vector<std::string>& getTags() const { return tags_; }

    /// @brief Returns the icon file relative to the node folder, empty when the node ships none.
    const std::string& getIconPath() const { return iconPath_; }

    /// @brief Returns the documentation file relative to the node folder, empty when there is
    /// none.
    const std::string& getDocsPath() const { return docsPath_; }

  private:
    NodeType nodeType_;
    NodeImplementation implementation_;
    std::vector<std::string> tags_;
    std::string iconPath_;
    std::string docsPath_;
};

} // namespace enzo::nt
