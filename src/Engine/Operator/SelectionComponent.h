#pragma once
#include <memory>
#include <string>
#include "Engine/Operator/IndexSet.h"
#include "Engine/Operator/Primitive.h"
#include "Engine/Types.h"

namespace enzo {
class SelectionComponent {
public:
  static SelectionComponent fromString(std::string_view string);
  bool containsPrim(const geo::Primitive& prim) const;
  bool containsFace(const geo::Primitive& prim, ga::Index index) const;

private:
  SelectionComponent() = default;
  std::string primPath_;
  std::shared_ptr<IndexSet> points_;
  std::shared_ptr<IndexSet> faces_;
  std::shared_ptr<IndexSet> vertices_;
};
} // namespace enzo
