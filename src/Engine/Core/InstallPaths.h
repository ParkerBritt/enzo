#pragma once
#include <filesystem>

namespace enzo {

/// @brief Returns the directory holding bin, lib, nodes and share.
std::filesystem::path getInstallRoot();

/// @brief Returns the directory holding the fonts, icons and theme.
///
/// @note Falls back to the source tree when the application runs from a build
/// directory rather than an install.
std::filesystem::path getStaticDir();

} // namespace enzo
