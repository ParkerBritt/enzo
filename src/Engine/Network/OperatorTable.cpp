#include "Engine/Network/OperatorTable.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/OpInfo.h"
#include <boost/dll/import.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem/file_status.hpp>
#include <icecream.hpp>

#include <boost/dll/shared_library.hpp>
#include <iostream>
#include <stdexcept>

namespace enzo {

void op::OperatorTable::addOperator(op::OpInfo info)
{
    std::cout << "OPERATOR TABLE ADDED\n";
    std::cout << "adding operator: " << info.displayName << "\n";

    for (const prm::Template& templateEntry : info.templates)
    {
        std::cout << "name: " << templateEntry.getName() << "\n";
    }

    opInfoStore_.push_back(info);
}

nt::opConstructor op::OperatorTable::getOpConstructor(std::string name)
{
    for (auto it = opInfoStore_.begin(); it != opInfoStore_.end(); ++it)
    {
        if (it->internalName == name)
        {
            return it->ctorFunc;
        }
    }
    return nullptr;
}

const std::optional<op::OpInfo> op::OperatorTable::getOpInfo(std::string name)
{
    for (auto it = opInfoStore_.begin(); it != opInfoStore_.end(); ++it)
    {
        if (it->internalName == name)
        {
            return *it;
        }
    }
    return std::nullopt;
}

std::vector<op::OpInfo> op::OperatorTable::getData() { return opInfoStore_; }

boost::filesystem::path op::OperatorTable::findPlugin(const std::string& undecoratedLibName)
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

void op::OperatorTable::initPlugins()
{
    static bool pluginsLoaded = false;
    if (pluginsLoaded) return;

    using InitPluginFn = void(op::addOperatorPtr);

    const auto so = findPlugin("enzoOps1");
    static boost::dll::shared_library lib(so, boost::dll::load_mode::default_mode);

    auto initPlugin = lib.get<InitPluginFn>("newSopOperator");
    initPlugin(op::OperatorTable::addOperator);

    pluginsLoaded = true;
}

std::vector<op::OpInfo> op::OperatorTable::opInfoStore_;

} // namespace enzo
