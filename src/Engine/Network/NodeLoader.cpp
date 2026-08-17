#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeManifest.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Network/NodeType.h"
#include "Engine/Network/NodeTypeTable.h"
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/dll/shared_library.hpp>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

// The source and build tree locations, which let a development run work with no install step.
#ifndef ENZO_DEV_NODES_DIR
#define ENZO_DEV_NODES_DIR ""
#endif
#ifndef ENZO_DEV_LIB_DIR
#define ENZO_DEV_LIB_DIR ""
#endif

namespace enzo::nt {

namespace {

// The file naming a folder as a node.
constexpr const char* kManifestName = "node.yaml";

// The directory the application is installed into, holding bin, lib and nodes.
std::filesystem::path getInstallRoot()
{
    const std::filesystem::path executable(boost::dll::program_location().string());
    return executable.parent_path().parent_path();
}

std::filesystem::path getLibraryFile(const std::string& libraryName)
{
    // TODO: add env var finder
    // TODO: add the node's own bin directory
    const std::string fileName = boost::dll::shared_library::decorate(libraryName).string();

    const std::filesystem::path installed = getInstallRoot() / "lib" / fileName;
    if (std::filesystem::exists(installed)) return installed;

    const std::filesystem::path developmentDirectory(ENZO_DEV_LIB_DIR);
    const std::filesystem::path development = developmentDirectory / fileName;
    if (!developmentDirectory.empty() && std::filesystem::exists(development)) return development;

    throw std::runtime_error(
        "Couldn't find library " + libraryName + ", tried " + installed.string() + " and " +
        development.string()
    );
}

boost::dll::shared_library& openLibrary(const std::string& libraryName)
{
    static std::map<std::string, boost::dll::shared_library> openLibraries;

    auto found = openLibraries.find(libraryName);
    if (found != openLibraries.end()) return found->second;

    const std::filesystem::path path = getLibraryFile(libraryName);
    auto inserted = openLibraries.emplace(
        libraryName,
        boost::dll::shared_library(path.string(), boost::dll::load_mode::default_mode)
    );
    return inserted.first->second;
}

// Get a nodes constructor from the library pointed at in the manifest file.
nodeConstructor getConstructor(const NodeImplementation& implementation)
{
    boost::dll::shared_library& library = openLibrary(implementation.library);
    const std::string symbol = nodeConstructorSymbol(implementation.constructor);

    if (!library.has(symbol))
        throw std::runtime_error(
            "library " + implementation.library + " exports no constructor named " +
            implementation.constructor
        );

    return &library.get<NodeImpl*(Node&, CookContext&)>(symbol);
}

// Reads one folder's manifest and registers the node type it describes.
void loadNode(const std::filesystem::path& folder)
{
    const std::filesystem::path manifestPath = folder / kManifestName;
    if (!std::filesystem::exists(manifestPath))
    {
        std::cerr << "Skipping " << folder.string() << ", it has no " << kManifestName << "\n";
        return;
    }

    const NodeManifest manifest = NodeManifest::loadFromFile(manifestPath);

    NodeType nodeType = manifest.getNodeType();
    nodeType.folder = folder;
    nodeType.ctorFunc = getConstructor(manifest.getImplementation());

    NodeTypeTable::addNodeType(std::move(nodeType));
}

} // namespace

std::filesystem::path NodeLoader::getNodesDirectory()
{
    const std::filesystem::path installed = getInstallRoot() / "nodes";
    if (std::filesystem::is_directory(installed)) return installed;

    const std::filesystem::path development(ENZO_DEV_NODES_DIR);
    if (!development.empty() && std::filesystem::is_directory(development)) return development;

    throw std::runtime_error(
        "Couldn't find a nodes directory, tried " + installed.string() + " and " +
        development.string()
    );
}

void NodeLoader::loadNodes()
{
    static bool nodesLoaded = false;
    if (nodesLoaded) return;
    nodesLoaded = true;

    for (const auto& entry : std::filesystem::directory_iterator(getNodesDirectory()))
    {
        // A node is always a folder.
        if (!entry.is_directory()) continue;

        // One broken node should not take the rest of them down with it.
        try
        {
            loadNode(entry.path());
        }
        catch (const std::exception& error)
        {
            std::cerr << "Couldn't load node " << entry.path().string() << ", " << error.what()
                      << "\n";
        }
    }
}

} // namespace enzo::nt
