#include "Engine/Selection/Selection.h"
#include "Engine/Primitives/Mesh.h"
#include "Engine/Selection/SelectionComponent.h"
#include <icecream.hpp>
#include <sstream>

namespace enzo {

Selection::Selection(std::string expression)
{
    char delimeter = ',';
    std::stringstream expressionString(expression);
    std::string stringPart;
    while (getline(expressionString, stringPart, delimeter))
    {
        IC(stringPart);
        components_.push_back(SelectionComponent::fromString(stringPart));
    }
    // Empty expression means "everything": one pathless, no-selectors component.
    if (components_.empty())
    {
        components_.push_back(SelectionComponent::fromString(""));
    }
    // for(size_t i = 0; i<expression.size(); ++i)
    // {
    //     char character = expression[i];
    //     selectionComponentString += character;
    //
    //     if(i == expression.size()-1 || expression[i+1] == ',')
    //     {
    //         SelectionComponent component =
    //         SelectionComponent::fromString(selectionComponentString);
    //         components_.push_back(component);
    //         selectionComponentString.clear();
    //     }
    // }
}

std::vector<std::shared_ptr<geo::Primitive>> Selection::getPrims(const NodePacket& packet)
{
    std::vector<std::shared_ptr<geo::Primitive>> prims;

    for (auto prim : packet.getPrimitives())
    {
        if (containsPrim(prim))
        {
            prims.push_back(prim);
        }
    }

    return prims;
}

bool Selection::containsPrim(geo::PrimPtr prim, bool full)
{
    if (inverted_)
    {
        // Inverted: a prim is fully contained only if no component mentions it;
        // partially contained if no component selects it as a whole prim.
        for (auto& component : components_)
        {
            if (!component->containsPrim(*prim)) continue;
            if (full || component->isWholePrim(*prim)) return false;
        }
        return true;
    }
    for (auto& component : components_)
    {
        if (!component->containsPrim(*prim)) continue;
        if (full && !component->isWholePrim(*prim)) continue;
        return true;
    }
    return false;
}

bool Selection::containsFace(geo::PrimPtr prim, Index index, Offset offset)
{
    bool addressed = false;
    bool member = false;
    for (auto& component : components_)
    {
        // A component "addresses" faces when it speaks about them at all: either
        // it selects this face directly, or inverted it selects the complement.
        if (component->containsFace(*prim, index, offset, false))
        {
            member = true;
            addressed = true;
        }
        else if (component->containsFace(*prim, index, offset, true))
        {
            addressed = true;
        }
    }
    // Plain selection is the union of members. Inverting takes the complement of
    // that union, but only across element types some component actually addresses.
    return inverted_ ? (addressed && !member) : member;
}

std::vector<Offset> Selection::getFaces(geo::PrimPtr prim)
{
    std::vector<Offset> result;

    // Ensure prim is a mesh
    auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
    if (!mesh) return result;

    // Walk valid faces and collect those in the selection
    const Offset storageSize = mesh->getNumFaces();
    Index index = 0;
    for (Offset offset = 0; offset < storageSize; ++offset)
    {
        if (!mesh->isValidFace(offset)) continue;
        if (containsFace(prim, index, offset))
        {
            result.push_back(offset);
        }
        ++index;
    }
    return result;
}

bool Selection::containsPoint(geo::PrimPtr prim, Index index, Offset offset)
{
    bool addressed = false;
    bool member = false;
    for (auto& component : components_)
    {
        if (component->containsPoint(*prim, index, offset, false))
        {
            member = true;
            addressed = true;
        }
        else if (component->containsPoint(*prim, index, offset, true))
        {
            addressed = true;
        }
    }
    return inverted_ ? (addressed && !member) : member;
}

std::vector<Offset> Selection::getPoints(geo::PrimPtr prim)
{
    std::vector<Offset> result;
    if (!prim) return result;

    // Walk valid points and collect those in the selection
    Index index = 0;
    for (Offset offset : prim->getPoints())
    {
        if (containsPoint(prim, index, offset))
        {
            result.push_back(offset);
        }
        ++index;
    }
    return result;
}

bool Selection::containsVertex(geo::PrimPtr prim, Index index, Offset offset)
{
    bool addressed = false;
    bool member = false;
    for (auto& component : components_)
    {
        if (component->containsVertex(*prim, index, offset, false))
        {
            member = true;
            addressed = true;
        }
        else if (component->containsVertex(*prim, index, offset, true))
        {
            addressed = true;
        }
    }
    return inverted_ ? (addressed && !member) : member;
}

std::vector<Offset> Selection::getVertices(geo::PrimPtr prim)
{
    std::vector<Offset> result;

    // Ensure prim is a mesh
    auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
    if (!mesh) return result;

    // Walk valid vertices and collect those in the selection
    const Offset storageSize = mesh->getNumVerts();
    Index index = 0;
    for (Offset offset = 0; offset < storageSize; ++offset)
    {
        if (!mesh->isValidVertex(offset)) continue;
        if (containsVertex(prim, index, offset))
        {
            result.push_back(offset);
        }
        ++index;
    }
    return result;
}

} // namespace enzo
