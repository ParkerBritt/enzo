#pragma once
#include "Engine/Network/NodeDef.h"
#include "Engine/Parameter/Template.h"

class GopGeometryImport : public enzo::nt::NodeDef
{
  public:
    GopGeometryImport(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType);
    virtual void cook(enzo::nt::CookContext context);
    static enzo::nt::NodeDef* ctor(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType)
    {
        return new GopGeometryImport(network, nodeType);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();
};
