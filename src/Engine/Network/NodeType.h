#pragma once
#include "Engine/Parameter/Template.h"
#include <string>
#include <vector>

namespace enzo::nt {
struct NodeType;
class NodeDef;
class NetworkManager;

using nodeConstructor = NodeDef* (*)(NetworkManager* network, NodeType nodeType);

struct NodeType
{
    std::string internalName;
    std::string displayName;
    nodeConstructor ctorFunc;
    std::vector<enzo::prm::Template> templates;
    unsigned int minInputs = 0;
    unsigned int maxInputs = 1;
    unsigned int maxOutputs = 1;

    /// @brief Returns the internal type name shared by all nodes of this type (eg.
    /// "copy_to_points")
    const std::string& getName() const { return internalName; }
    /// @brief Returns the human readable type label shown in the UI (eg. "Copy To Points")
    const std::string& getLabel() const { return displayName; }
};
} // namespace enzo::nt
