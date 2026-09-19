#include "Engine/Network/NodeLoader.h"
#include "Engine/Core/InstallPaths.h"
#include "Engine/Network/NodeManifest.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Network/NodeType.h"
#include "Engine/Network/NodeTypeTable.h"
#include <boost/dll/shared_library.hpp>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

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

// Returns a node's constructor from the library its manifest names.
nodeConstructor getConstructor(const CppImplementation& implementation)
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

// A node folder on disk and the manifest read from it.
struct NodeFolder
{
    std::filesystem::path path;
    NodeManifest manifest;
};

void reportSkippedNode(const std::filesystem::path& folder, const std::exception& error)
{
    std::cerr << "Couldn't load node " << folder.string() << ", " << error.what() << "\n";
}

// Returns every node folder with its manifest. A folder that fails to read is
// reported and skipped.
std::vector<NodeFolder> readNodeFolders()
{
    std::vector<NodeFolder> nodeFolders;
    for (const auto& entry : std::filesystem::directory_iterator(NodeLoader::getNodesDirectory()))
    {
        // A node is always a folder.
        if (!entry.is_directory()) continue;

        const std::filesystem::path manifestPath = entry.path() / kManifestName;
        if (!std::filesystem::exists(manifestPath))
        {
            std::cerr << "Skipping " << entry.path().string() << ", it has no " << kManifestName
                      << "\n";
            continue;
        }

        try
        {
            nodeFolders.push_back({entry.path(), NodeManifest::loadFromFile(manifestPath)});
        }
        catch (const std::exception& error)
        {
            reportSkippedNode(entry.path(), error);
        }
    }
    return nodeFolders;
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

    const std::vector<NodeFolder> nodeFolders = readNodeFolders();

    // Nodes with their own implementation
    for (const NodeFolder& nodeFolder : nodeFolders)
    {
        const auto* implementation =
            std::get_if<CppImplementation>(&nodeFolder.manifest.getImplementation());
        if (!implementation) continue;

        try
        {
            NodeType nodeType = nodeFolder.manifest.getNodeType();
            nodeType.folder = nodeFolder.path;
            nodeType.ctorFunc = getConstructor(*implementation);
            NodeTypeTable::addNodeType(std::move(nodeType));
        }
        catch (const std::exception& error)
        {
            reportSkippedNode(nodeFolder.path, error);
        }
    }

    // Aliases of the node types registered above
    for (const NodeFolder& nodeFolder : nodeFolders)
    {
        const auto* implementation =
            std::get_if<AliasImplementation>(&nodeFolder.manifest.getImplementation());
        if (!implementation) continue;

        try
        {
            const NodeType& aliasedType =
                NodeTypeTable::requireNodeType(implementation->aliasedType);
            NodeTypeTable::addNodeAlias(nodeFolder.manifest.getNodeAlias(aliasedType));
        }
        catch (const std::exception& error)
        {
            reportSkippedNode(nodeFolder.path, error);
        }
    }
}

} // namespace enzo::nt
