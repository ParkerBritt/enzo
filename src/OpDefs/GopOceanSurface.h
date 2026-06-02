#pragma once
#include "Engine/Network/GeometryOpDef.h"
#include "Engine/Parameter/Template.h"
#include "Engine/Core/Types.h"

class GopOceanSurface
: public enzo::nt::GeometryOpDef
{
public:
    GopOceanSurface(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo);
    virtual void cookOp(enzo::op::Context context);
    enzo::Vector3 getSurfacePos(const enzo::Vector3 pos);
    static enzo::nt::GeometryOpDef* ctor(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    {
        return new GopOceanSurface(network, opInfo);
    }

    static BOOST_SYMBOL_EXPORT std::vector<enzo::prm::Template> parameterList();

};
