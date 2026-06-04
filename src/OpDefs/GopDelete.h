#pragma once
#include "Engine/Network/GeometryOpDef.h"
#include "Engine/Parameter/Template.h"

class GopDelete : public enzo::nt::GeometryOpDef
{
  public:
    GopDelete(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo);
    virtual void cookOp(enzo::op::Context context);
    static enzo::nt::GeometryOpDef* ctor(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    {
        return new GopDelete(network, opInfo);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();
};
