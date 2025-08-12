#pragma once

#include "Engine/Types.h"
namespace enzo::prm
{
class Name
{
public:
    Name(bt::String token, bt::String label);
    Name();

    bt::String getToken() const;
    bt::String getLabel() const;
private:
    bt::String token_;
    bt::String label_;
};

}
