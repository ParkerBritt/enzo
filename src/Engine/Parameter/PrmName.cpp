#include "Engine/Parameter/PrmName.h"

enzo::prm::Name::Name(String token, String label)
: token_{token}, label_{label}
{

}

enzo::prm::Name::Name()
: token_{""}, label_{""}
{

}

enzo::String enzo::prm::Name::getToken() const
{
    return token_;
}

enzo::String enzo::prm::Name::getLabel() const
{
    return label_;
}
