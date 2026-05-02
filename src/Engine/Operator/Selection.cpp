#include "Selection.h"
#include "SelectionComponent.h"
#include <icecream.hpp>
#include <sstream>

enzo::Selection::Selection(std::string expression)
{
    // SelectionComponent component = SelectionComponent::fromGroup();
    char delimeter = ',';
    std::stringstream expressionString(expression);
    std::string stringPart;
    while (getline(expressionString, stringPart, delimeter))
    {
        IC(stringPart);
        SelectionComponent component = SelectionComponent::fromString(stringPart);
        components_.push_back(component);
    }
    // for(size_t i = 0; i<expression.size(); ++i)
    // {
    //     char character = expression[i];
    //     selectionComponentString += character;
    //
    //     if(i == expression.size()-1 || expression[i+1] == ',')
    //     {
    //         SelectionComponent component = SelectionComponent::fromString(selectionComponentString);
    //         components_.push_back(component);
    //         selectionComponentString.clear();
    //     }
    // }
}

std::vector<std::shared_ptr<enzo::geo::Primitive>> enzo::Selection::getPrimitives(const NodePacket& packet)
{
    std::vector<std::shared_ptr<enzo::geo::Primitive>> prims;
    IC();

    for(auto prim : packet.getPrimitives())
    {
        for(auto component : components_)
        {
            if(component.containsPrim(*prim))
            {
                prims.push_back(prim);
                break;
            }

        }
    }

    return prims;

}
