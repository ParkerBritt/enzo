#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Network/NodeDef.h"
#include "Engine/Parameter/Template.h"

class GopOceanSurface : public enzo::nt::NodeDef
{
  public:
    GopOceanSurface(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType);
    virtual void cook(enzo::nt::CookContext context);
    enzo::Vector3 getSurfacePos(const enzo::Vector3 pos);
    static enzo::nt::NodeDef* ctor(enzo::nt::NetworkManager* network, enzo::nt::NodeType nodeType)
    {
        return new GopOceanSurface(network, nodeType);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();
};
