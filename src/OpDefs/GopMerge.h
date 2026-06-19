#pragma once
#include "Engine/Network/GeometryOpDef.h"
#include "Engine/Parameter/Template.h"

class GopMerge : public enzo::nt::GeometryOpDef
{
  public:
    GopMerge(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo);
    virtual void cookOp(enzo::op::CookContext context);
    static enzo::nt::GeometryOpDef* ctor(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    {
        return new GopMerge(network, opInfo);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();
};
