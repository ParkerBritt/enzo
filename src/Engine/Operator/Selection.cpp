#include "Selection.h"
#include "Mesh.h"
#include "SelectionComponent.h"
#include <icecream.hpp>
#include <sstream>

enzo::Selection::Selection(std::string expression) {
    char delimeter = ',';
    std::stringstream expressionString(expression);
    std::string stringPart;
    while (getline(expressionString, stringPart, delimeter)) {
        IC(stringPart);
        components_.push_back(SelectionComponent::fromString(stringPart));
    }
    // Empty expression means "everything": one pathless, no-selectors component.
    if (components_.empty()) {
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

std::vector<std::shared_ptr<enzo::geo::Primitive>>
enzo::Selection::getPrims(const NodePacket &packet) {
    std::vector<std::shared_ptr<enzo::geo::Primitive>> prims;

    for (auto prim : packet.getPrimitives()) {
        if (containsPrim(prim)) {
            prims.push_back(prim);
        }
    }

    return prims;
}

bool enzo::Selection::containsPrim(geo::PrimPtr prim, bool full) {
    if (inverted_) {
        // Inverted: a prim is fully contained only if no component mentions it;
        // partially contained if no component selects it as a whole prim.
        for (auto& component : components_) {
            if (!component->containsPrim(*prim)) continue;
            if (full || component->isWholePrim(*prim)) return false;
        }
        return true;
    }
    for (auto& component : components_) {
        if (!component->containsPrim(*prim)) continue;
        if (full && !component->isWholePrim(*prim)) continue;
        return true;
    }
    return false;
}

bool enzo::Selection::containsFace(geo::PrimPtr prim, ga::Index index) {
    // If the whole prim is in the selection, every face on it is in too.
    if (containsPrim(prim, true)) return true;
    for (auto& component : components_) {
        if (component->containsFace(*prim, index, inverted_)) return true;
    }
    return false;
}

std::vector<enzo::ga::Offset> enzo::Selection::getFaces(geo::PrimPtr prim) {
    std::vector<ga::Offset> result;

    // Ensure prim is a mesh
    auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
    if (!mesh) return result;

    // Walk valid faces and collect those in the selection
    const ga::Offset storageSize = mesh->getNumFaces();
    ga::Index index = 0;
    for (ga::Offset offset = 0; offset < storageSize; ++offset) {
        if (!mesh->isValidFace(offset)) continue;
        if (containsFace(prim, index)) {
            result.push_back(offset);
        }
        ++index;
    }
    return result;
}

bool enzo::Selection::containsPoint(geo::PrimPtr prim, ga::Index index) {
    if (containsPrim(prim, true)) return true;
    for (auto& component : components_) {
        if (component->containsPoint(*prim, index, inverted_)) return true;
    }
    return false;
}

std::vector<enzo::ga::Offset> enzo::Selection::getPoints(geo::PrimPtr prim) {
    std::vector<ga::Offset> result;
    if (!prim) return result;

    // Walk valid points and collect those in the selection
    ga::Index index = 0;
    for (ga::Offset offset : prim->getPoints()) {
        if (containsPoint(prim, index)) {
            result.push_back(offset);
        }
        ++index;
    }
    return result;
}

bool enzo::Selection::containsVertex(geo::PrimPtr prim, ga::Index index) {
    if (containsPrim(prim, true)) return true;
    for (auto& component : components_) {
        if (component->containsVertex(*prim, index, inverted_)) return true;
    }
    return false;
}

std::vector<enzo::ga::Offset> enzo::Selection::getVertices(geo::PrimPtr prim) {
    std::vector<ga::Offset> result;

    // Ensure prim is a mesh
    auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
    if (!mesh) return result;

    // Walk valid vertices and collect those in the selection
    const ga::Offset storageSize = mesh->getNumVerts();
    ga::Index index = 0;
    for (ga::Offset offset = 0; offset < storageSize; ++offset) {
        if (!mesh->isValidVertex(offset)) continue;
        if (containsVertex(prim, index)) {
            result.push_back(offset);
        }
        ++index;
    }
    return result;
}
