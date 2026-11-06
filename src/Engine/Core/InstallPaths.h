#pragma once
#include <filesystem>
#include <string>

namespace enzo {

/// @brief Returns the directory holding bin, lib, nodes and share.
std::filesystem::path getInstallRoot();

/// @brief Returns the directory holding the fonts, icons and theme.
///
/// @note Falls back to the source tree when the application runs from a build
/// directory rather than an install.
std::filesystem::path getStaticDir();

/// @brief Returns the SVG file for an icon name.
///
/// @note The Lucide set is searched before Enzo's own icons.
/// @return The file, empty when neither set has the icon.
std::filesystem::path getIconPath(const std::string& name);

} // namespace enzo
