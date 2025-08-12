#include "Engine/Parameter/PrmName.h"

enzo::prm::Name::Name(bt::String token, bt::String label)
: token_{token}, label_{label}
{

}

enzo::prm::Name::Name()
: token_{""}, label_{""}
{

}

enzo::bt::String enzo::prm::Name::getToken() const
{
    return token_;
}

enzo::bt::String enzo::prm::Name::getLabel() const
{
    return label_;
}
