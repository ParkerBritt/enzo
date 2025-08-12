#pragma once
#include "Engine/Operator/GeometryOpDef.h"
#include "Engine/Parameter/Template.h"

class GopSineWave
: public enzo::nt::GeometryOpDef
{
public:
    GopSineWave(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo);
    virtual void cookOp(enzo::op::Context context);
    static enzo::nt::GeometryOpDef* ctor(enzo::nt::NetworkManager* network, enzo::op::OpInfo opInfo)
    {
        return new GopSineWave(network, opInfo);
    }

    static BOOST_SYMBOL_EXPORT enzo::prm::Template parameterList[];

};
