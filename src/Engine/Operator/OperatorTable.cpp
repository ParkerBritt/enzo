#include "Engine/Operator/OperatorTable.h"
#include "Engine/Operator/OpInfo.h"
#include "Engine/Types.h"
#include <boost/dll/import.hpp>

#include <iostream>

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

void enzo::op::OperatorTable::initPlugins()
{
    static bool pluginsLoaded=false;
    if(pluginsLoaded) return;

    auto initPlugin = boost::dll::import_symbol<void(enzo::op::addOperatorPtr)>(
        "build/src/OpDefs/libenzoOps1.so", "newSopOperator"
    );

    initPlugin(enzo::op::OperatorTable::addOperator);

    pluginsLoaded = true;
}

std::vector<enzo::op::OpInfo> enzo::op::OperatorTable::opInfoStore_;
