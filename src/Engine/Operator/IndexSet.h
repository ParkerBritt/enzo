#pragma once
#include "Engine/Types.h"
#include <set>

namespace enzo {

class IndexSet {
  public:
    virtual ~IndexSet() = default;
    virtual bool contains(ga::Index index) const = 0;
};

class ExplicitIndexSet : public IndexSet {
  public:
    ExplicitIndexSet(std::set<ga::Index> indices);
    bool contains(ga::Index index) const override;

  private:
    std::set<ga::Index> indices_;
};

class WildcardIndexSet : public IndexSet {
  public:
    bool contains(ga::Index index) const override;
};

} // namespace enzo
