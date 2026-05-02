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
  std::vector<std::shared_ptr<geo::Primitive>> getPrimitives(const NodePacket& packet);
  bool containsFullPrimitive();
private:
  std::vector<enzo::SelectionComponent> components_;
};
} // namespace enzo
