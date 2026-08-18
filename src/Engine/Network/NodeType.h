#pragma once
#include "Engine/Parameter/Template.h"
#include <filesystem>
#include <string>
#include <vector>

namespace enzo::nt {
class Node;
class NodeImpl;
class CookContext;

/// @brief The exported function a node library gives out to build one cook's implementation.
using nodeConstructor = NodeImpl* (*)(Node&, CookContext&);

/**
 * @brief Everything every node of one kind shares.
 *
 * A node type is what a grid node has in common with every other grid node, so
 * the label, the parameters, the input and output counts, and the constructor
 * that produces its behaviour. It comes from a node folder's manifest and is
 * owned by the nt::NodeTypeTable, with each node holding a reference to it.
 */
struct NodeType
{
    std::string internalName;

    /// @brief The namespace the node was published under, such as "enzo".
    /// @note Two authors may both publish a "circle" as long as their namespaces differ.
    std::string typeNamespace;

    std::string displayName;
    nodeConstructor ctorFunc = nullptr;
    std::vector<enzo::prm::Template> templates;
    unsigned int minInputs = 0;
    unsigned int maxInputs = 1;
    unsigned int maxOutputs = 1;

    /// @brief The words the tab menu searches on, such as "curve" or "primitive".
    std::vector<std::string> tags;

    /// @brief The folder the node was loaded from, which is also where its assets live.
    std::filesystem::path folder;

    /// @brief The icon file relative to the folder, empty when the node ships none.
    std::string iconPath;

    /// @brief The documentation file relative to the folder, empty when there is none.
    std::string docsPath;

    /**
     * @brief The kind of scope this node holds inside it, empty when it holds none.
     *
     * A node naming one can be entered and filled with other nodes, and the kind names
     * what those nodes are allowed to be, so a "geometry" scope takes geometry nodes.
     */
    std::string childScopeType;

    /// @brief Returns the internal type name shared by all nodes of this type (eg.
    /// "copy_to_points")
    const std::string& getName() const { return internalName; }
    /// @brief Returns the name that uniquely identifies this type (eg. "enzo::copy_to_points")
    /// @note This is the form saved scenes store and the form lookups take.
    std::string getFullName() const { return typeNamespace + "::" + internalName; }
    /// @brief Returns the human readable type label shown in the UI (eg. "Copy To Points")
    const std::string& getLabel() const { return displayName; }

    /// @brief Returns whether this node holds a scope of other nodes inside it.
    bool hasChildScope() const { return !childScopeType.empty(); }

    /// @brief Returns the icon file on disk.
    /// @return The full path, empty when the node ships no icon.
    std::filesystem::path getIconFile() const
    {
        if (iconPath.empty()) return {};
        return folder / iconPath;
    }
};
} // namespace enzo::nt
