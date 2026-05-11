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
  // When `inverted` is true the answer is flipped, but only for the
  // primitive part the component actually talks about (f{...}, p{...}, v{...}).
  bool containsFace(const geo::Primitive& prim, ga::Index index, bool inverted = false) const;
  bool containsPoint(const geo::Primitive& prim, ga::Index index, bool inverted = false) const;
  bool containsVertex(const geo::Primitive& prim, ga::Index index, bool inverted = false) const;
  bool isWholePrim() const;

private:
  SelectionComponent() = default;
  std::string primPath_;
  std::shared_ptr<IndexSet> points_;
  std::shared_ptr<IndexSet> faces_;
  std::shared_ptr<IndexSet> vertices_;
};
} // namespace enzo
