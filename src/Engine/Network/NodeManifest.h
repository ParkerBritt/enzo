#pragma once
#include "Engine/Network/NodeAlias.h"
#include "Engine/Network/NodeType.h"
#include <filesystem>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace enzo::nt {

/// @brief The constructor a shared library exports to give a node its behaviour.
struct CppImplementation
{
    std::string library;
    std::string constructor;
};

/// @brief The node type an alias creates in its place.
struct AliasImplementation
{
    /// @brief The full name of the node type the alias stands in for, such as
    /// "enzo::attributeNoise".
    std::string aliasedType;
};

/// @brief Where a node's behaviour comes from.
using NodeImplementation = std::variant<CppImplementation, AliasImplementation>;

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
 * manifest.getNodeType().getLabel();                                   // "Sweep"
 * std::get<CppImplementation>(manifest.getImplementation()).constructor; // "sweep"
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
    /// @throws std::runtime_error when the document is malformed.
    static NodeManifest loadFromString(const std::string& yaml);

    /// @brief Returns the manifest described by a node.yaml file.
    /// @throws std::runtime_error when the file is missing or malformed.
    static NodeManifest loadFromFile(const std::filesystem::path& path);

    /// @brief Returns the node type this manifest describes, with no constructor or folder
    /// filled in yet.
    const NodeType& getNodeType() const { return nodeType_; }
    const NodeImplementation& getImplementation() const { return implementation_; }

    /**
     * @brief Returns the parameter values an alias sets, as written in the manifest.
     *
     * `offset: [0, 1, 0]` gives `{"offset", {"0", "1", "0"}}`.
     */
    const std::map<std::string, std::vector<std::string>>& getParameterValues() const
    {
        return parameterValues_;
    }

    /**
     * @brief Returns the alias this manifest describes.
     *
     * @throws std::runtime_error when a value does not fit a parameter of the aliased type.
     *
     * @param aliasedType The node type the alias stands in for, whose parameters give each
     * value its type.
     */
    NodeAlias getNodeAlias(const NodeType& aliasedType) const;

  private:
    NodeType nodeType_;
    NodeImplementation implementation_;
    std::map<std::string, std::vector<std::string>> parameterValues_;
};

} // namespace enzo::nt
