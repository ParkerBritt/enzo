#pragma once

#include "Engine/Network/NodeDef.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeType.h"
#include "Engine/Parameter/Template.h"
#include <boost/config.hpp>
#include <boost/filesystem.hpp>

namespace enzo::nt {

class BOOST_SYMBOL_EXPORT NodeTypeTable
{
  public:
    static void addNodeType(enzo::nt::NodeType nodeType);
    static nt::nodeConstructor getNodeConstructor(std::string name);
    static const std::optional<nt::NodeType> getNodeType(std::string name);
    static std::vector<NodeType> getData();
    static boost::filesystem::path findPlugin(const std::string& undecoratedLibName);
    // TODO: move to better spot (maybe engine class)
    static void initPlugins();

  private:
    static std::vector<NodeType> nodeTypeStore_;
};
using addNodeTypePtr = void (*)(NodeType);
} // namespace enzo::nt
