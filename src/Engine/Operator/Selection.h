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
    bool containsFace(geo::PrimPtr prim, Index index, Offset offset);
    bool containsPoint(geo::PrimPtr prim, Index index, Offset offset);
    bool containsVertex(geo::PrimPtr prim, Index index, Offset offset);
    std::vector<Offset> getFaces(geo::PrimPtr prim);
    std::vector<Offset> getPoints(geo::PrimPtr prim);
    std::vector<Offset> getVertices(geo::PrimPtr prim);

    void setInverted(bool inverted) { inverted_ = inverted; }
    bool getInverted() const { return inverted_; }

  private:
    std::vector<std::unique_ptr<enzo::SelectionComponent>> components_;
    bool inverted_ = false;
};
} // namespace enzo
