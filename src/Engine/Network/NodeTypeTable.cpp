#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeType.h"
#include <boost/dll/import.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem/file_status.hpp>
#include <icecream.hpp>

#include <boost/dll/shared_library.hpp>
#include <iostream>
#include <stdexcept>

namespace enzo {

void nt::NodeTypeTable::addNodeType(nt::NodeType nodeType)
{
    std::cout << "NODE TYPE TABLE ADDED\n";
    std::cout << "adding node type: " << nodeType.displayName << "\n";

    for (const prm::Template& templateEntry : nodeType.templates)
    {
        std::cout << "name: " << templateEntry.getName() << "\n";
    }

    nodeTypeStore_.push_back(nodeType);
}

nt::nodeConstructor nt::NodeTypeTable::getNodeConstructor(std::string name)
{
    for (auto it = nodeTypeStore_.begin(); it != nodeTypeStore_.end(); ++it)
    {
        if (it->internalName == name)
        {
            return it->ctorFunc;
        }
    }
    return nullptr;
}

const std::optional<nt::NodeType> nt::NodeTypeTable::getNodeType(std::string name)
{
    for (auto it = nodeTypeStore_.begin(); it != nodeTypeStore_.end(); ++it)
    {
        if (it->internalName == name)
        {
            return *it;
        }
    }
    return std::nullopt;
}

std::vector<nt::NodeType> nt::NodeTypeTable::getData() { return nodeTypeStore_; }

boost::filesystem::path nt::NodeTypeTable::findPlugin(const std::string& undecoratedLibName)
{

    const auto libName = boost::dll::shared_library::decorate(undecoratedLibName);

    // check for lib dir
    {
        const boost::filesystem::path executable = boost::dll::program_location();
        const boost::filesystem::path enzoRoot = executable.parent_path().parent_path();
        const boost::filesystem::path enzoLib = enzoRoot / "lib";
        const boost::filesystem::path candidate = enzoLib / libName;

        if (boost::filesystem::exists(candidate))
        {
            IC(candidate);
            return candidate;
        }
        else
            std::cout << "Couldn't find lib at: " << candidate.string() << "\n";
    }

// check for dev macro
#ifndef ENZO_DEV_LIB_DIR
#define ENZO_DEV_LIB_DIR ""
#endif
    if (std::string(ENZO_DEV_LIB_DIR).size())
    {
        const auto candidate = boost::filesystem::path(ENZO_DEV_LIB_DIR) / libName;
        if (boost::filesystem::exists(candidate))
        {
            IC(candidate);
            return candidate;
        }
        else
            std::cout << "Couldn't find lib at: " << candidate.string() << "\n";
    }

    // TODO: add env var finder
    // TODO: add same dirfinder

    throw std::runtime_error("Couldn't find plugin: " + libName.string());
}

void nt::NodeTypeTable::initPlugins()
{
    static bool pluginsLoaded = false;
    if (pluginsLoaded) return;

    using InitPluginFn = void(nt::addNodeTypePtr);

    const auto so = findPlugin("enzoOps1");
    // Held open for the life of the program to avoid unloading in the wrong
    // order. Freed by OS.
    static auto& lib = *new boost::dll::shared_library(so, boost::dll::load_mode::default_mode);

    auto initPlugin = lib.get<InitPluginFn>("newNodeLibrary");
    initPlugin(nt::NodeTypeTable::addNodeType);

    pluginsLoaded = true;
}

std::vector<nt::NodeType> nt::NodeTypeTable::nodeTypeStore_;

} // namespace enzo
