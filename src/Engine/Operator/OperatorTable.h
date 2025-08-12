#pragma once

#include <boost/config.hpp>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOpDef.h"
#include "Engine/Operator/OpInfo.h"
#include "Engine/Parameter/Template.h"
#include <boost/filesystem.hpp>


namespace enzo::op
{

class BOOST_SYMBOL_EXPORT OperatorTable
{
public:
    static void addOperator(enzo::op::OpInfo info);
    static nt::opConstructor getOpConstructor(std::string name);
    static const std::optional<op::OpInfo> getOpInfo(std::string name);
    static std::vector<OpInfo> getData();
    static boost::filesystem::path findPlugin(const std::string& undecoratedLibName);
    // TODO: move to better spot (maybe engine class)
    static void initPlugins();
private:
    static std::vector<OpInfo> opInfoStore_;
};
using addOperatorPtr = void (*)(OpInfo);
}

