#include "Engine/Core/InstallPaths.h"
#include <boost/dll/runtime_symbol_info.hpp>

// The source tree location, which lets a development run work with no install step.
#ifndef ENZO_DEV_STATIC_DIR
#define ENZO_DEV_STATIC_DIR ""
#endif

namespace enzo {

std::filesystem::path getInstallRoot()
{
    const std::filesystem::path executable(boost::dll::program_location().string());
    return executable.parent_path().parent_path();
}

std::filesystem::path getStaticDir()
{
    const std::filesystem::path installed = getInstallRoot() / "share";
    if (std::filesystem::exists(installed)) return installed;
    return ENZO_DEV_STATIC_DIR;
}

} // namespace enzo
