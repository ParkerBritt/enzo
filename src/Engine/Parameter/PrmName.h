#pragma once

#include "Engine/Types.h"
namespace enzo::prm
{
class Name
{
public:
    Name(String token, String label);
    Name();

    String getToken() const;
    String getLabel() const;
private:
    String token_;
    String label_;
};

}
