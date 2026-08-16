#pragma once
#include "Engine/Network/NodeDef.h"
#include "Engine/Parameter/Template.h"

class GOP_house : public enzo::nt::NodeDef
{
  public:
    GOP_house(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType);
    virtual void cook(enzo::nt::CookContext context);
    static enzo::nt::NodeDef* ctor(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType)
    {
        return new GOP_house(network, nodeType);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();
};
