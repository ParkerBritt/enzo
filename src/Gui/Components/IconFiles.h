#pragma once

#include "Engine/Network/NodeType.h"
#include <QString>
#include <QUrl>
#include <filesystem>

namespace enzo::ui {

/// @brief Returns the SVG file of an icon by name.
///
/// `getIconFile("plus")` gives the Lucide plus icon, and `getIconFile("node/enzo::grid")`
/// gives the grid node's icon.
///
/// @return The path, empty when no icon has the name.
std::filesystem::path getIconFile(const QString& name);

/// @brief Returns the SVG file of a node type's icon, or the default node icon when it ships none.
std::filesystem::path getNodeIconFile(const nt::NodeType& nodeType);

/// @brief Returns the file url of a node type's icon.
QUrl getNodeIconUrl(const nt::NodeType& nodeType);

} // namespace enzo::ui
