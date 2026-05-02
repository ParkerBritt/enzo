#include "SelectionComponent.h"
#include <icecream.hpp>


enzo::SelectionComponent enzo::SelectionComponent::fromString(std::string_view string)
{
    enzo::SelectionComponent component;
    component.primPath_ = string;

    // std::string tokenString;
    // for(size_t i =1; i< string.size(); ++i)
    // {
    //     char character = string[i];
    //     tokenString += character;
    //
    //     if(i==string.size()-1 || std::isspace(string[i+1]))
    //     {
    //         component.primPath_ = tokenString;
    //     }
    // }

    // component.explicitPoints_ = ;
    // component.explicitFaces_;
    // component.explicitVertices_;

    return component;
}


bool enzo::SelectionComponent::containsPrim(const geo::Primitive& prim)
{
    IC(prim.getPath(), primPath_);
    return prim.getPath() == primPath_;
}
