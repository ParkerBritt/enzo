#include "Engine/Operator/OperatorTable.h"
#include "Engine/Operator/OpInfo.h"
#include "Engine/Types.h"
#include <boost/dll/import.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem/file_status.hpp>
#include <icecream.hpp>

#include <boost/dll/shared_library.hpp>
#include <iostream>
#include <stdexcept>

void enzo::op::OperatorTable::addOperator(enzo::op::OpInfo info)
{
    std::cout << "OPERATOR TABLE ADDED\n";
    std::cout << "adding operator: " << info.displayName << "\n";

    for(const prm::Template* t = info.templates; t->getType()!=prm::Type::LIST_TERMINATOR; ++t)
    {
        std::cout << "name: " << t->getName() << "\n";
    }


    opInfoStore_.push_back(info);
}

enzo::nt::opConstructor enzo::op::OperatorTable::getOpConstructor(std::string name)
{
    for(auto it = opInfoStore_.begin(); it!=opInfoStore_.end(); ++it)
    {
        if(it->internalName==name)
        {
            return it->ctorFunc;
        }
    }
    return nullptr;
}

const std::optional<enzo::op::OpInfo> enzo::op::OperatorTable::getOpInfo(std::string name)
{
    for(auto it = opInfoStore_.begin(); it!=opInfoStore_.end(); ++it)
    {
        if(it->internalName==name)
        {
            return *it;
        }
    }
    return std::nullopt;
}

std::vector<enzo::op::OpInfo> enzo::op::OperatorTable::getData()
{
    return opInfoStore_;
}

boost::filesystem::path enzo::op::OperatorTable::findPlugin(const std::string& undecoratedLibName)
{

    const auto libName = boost::dll::shared_library::decorate(undecoratedLibName);

    // check for lib dir
    {
        const boost::filesystem::path executable = boost::dll::program_location();
        const boost::filesystem::path enzoRoot = executable.parent_path().parent_path();
        const boost::filesystem::path enzoLib = enzoRoot / "lib";
        const boost::filesystem::path candidate = enzoLib / libName;

        if(boost::filesystem::exists(candidate))
        {
            IC(candidate);
            return candidate;
        }
        else std::cout << "Couldn't find lib at: " << candidate.string() << "\n"; 
    }

    // check for dev macro
    #ifndef ENZO_DEV_LIB_DIR
        #define ENZO_DEV_LIB_DIR ""
    #endif
    if(std::string(ENZO_DEV_LIB_DIR).size())
    {
        const auto candidate = boost::filesystem::path(ENZO_DEV_LIB_DIR) / libName;
        if(boost::filesystem::exists(candidate))
        {
            IC(candidate);
            return candidate;
        }
        else std::cout << "Couldn't find lib at: " << candidate.string() << "\n"; 
    }


    // TODO: add env var finder
    // TODO: add same dirfinder

    throw std::runtime_error("Couldn't find plugin: " + libName.string());
}


void enzo::op::OperatorTable::initPlugins()
{
    static bool pluginsLoaded=false;
    if(pluginsLoaded) return;

    // auto initPlugin = boost::dll::import_symbol<void(enzo::op::addOperatorPtr)>(
    //     "build/src/OpDefs/libenzoOps1.so", "newSopOperator"
    // );
    auto initPlugin = boost::dll::import_symbol<void(enzo::op::addOperatorPtr)>(
        findPlugin("enzoOps1"), "newSopOperator"
    );

    initPlugin(enzo::op::OperatorTable::addOperator);

    pluginsLoaded = true;
}

std::vector<enzo::op::OpInfo> enzo::op::OperatorTable::opInfoStore_;
