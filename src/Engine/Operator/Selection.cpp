#include "Selection.h"
#include "Mesh.h"
#include "SelectionComponent.h"
#include <icecream.hpp>
#include <sstream>

enzo::Selection::Selection(std::string expression) {
    // SelectionComponent component = SelectionComponent::fromGroup();
    char delimeter = ',';
    std::stringstream expressionString(expression);
    std::string stringPart;
    while (getline(expressionString, stringPart, delimeter)) {
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
        for (auto component : components_) {
            if (component.containsPrim(*prim)) {
                prims.push_back(prim);
                break;
            }
        }
    }

    return prims;
}

bool enzo::Selection::containsPrim(geo::PrimPtr prim, bool full) {
    for (auto& component : components_) {
        if (!component.containsPrim(*prim)) continue;
        if (full && !component.isWholePrim()) continue;
        return true;
    }
    return false;
}

bool enzo::Selection::containsFace(geo::PrimPtr prim, ga::Index index) {
    for (auto& component : components_) {
        if (component.containsFace(*prim, index)) {
            return true;
        }
    }
    return false;
}

std::vector<enzo::ga::Offset> enzo::Selection::getFaces(geo::PrimPtr prim) {
    std::vector<ga::Offset> result;

    // Ensure prim is a mesh
    auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);
    if (!mesh) return result;

    // Walk valid faces and collect those in the selection
    const ga::Offset storageSize = mesh->getNumPrims();
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
    for (auto& component : components_) {
        if (component.containsPoint(*prim, index)) {
            return true;
        }
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
    for (auto& component : components_) {
        if (component.containsVertex(*prim, index)) {
            return true;
        }
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
