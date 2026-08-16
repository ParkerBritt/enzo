#pragma once
#include "Engine/Network/NodeDef.h"
#include "Engine/Parameter/Template.h"

class GopCamera : public enzo::nt::NodeDef
{
  public:
    GopCamera(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType);
    virtual void cook(enzo::nt::CookContext context);
    static enzo::nt::NodeDef* ctor(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType)
    {
        return new GopCamera(network, nodeType);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();
};
