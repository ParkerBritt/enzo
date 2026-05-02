#pragma once
#include <set>
#include <string>
#include "Engine/Operator/Primitive.h"
#include "Engine/Types.h"

namespace enzo {
class SelectionComponent {
public:
  static SelectionComponent fromString(std::string_view string);
  bool containsPrim(const geo::Primitive& prim);
private:
  void parsePath(std::string_view string);
  void parseIndexBlock(std::string_view string);
  SelectionComponent() = default;
  std::string primPath_;
  std::set<ga::Index> explicitPoints_;
  std::set<ga::Index> explicitFaces_;
  std::set<ga::Index> explicitVertices_;
};
} // namespace enzo
