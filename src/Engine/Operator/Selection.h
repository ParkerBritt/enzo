#pragma once
#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Primitive.h"
#include "SelectionComponent.h"
#include <memory>
#include <string>

namespace enzo {
class Selection {
  public:
    Selection(std::string expression);

    void getPoints();
    void getFaces();
    void getVertices();
    std::vector<std::shared_ptr<geo::Primitive>> getPrims(const NodePacket &packet);
    bool containsPrim(geo::PrimPtr prim, bool full = false);

  private:
    std::vector<enzo::SelectionComponent> components_;
};
} // namespace enzo
