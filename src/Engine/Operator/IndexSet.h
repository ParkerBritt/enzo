#pragma once
#include "Engine/Types.h"
#include <set>

namespace enzo {

class IndexSet {
  public:
    virtual ~IndexSet() = default;
    virtual bool contains(attr::Index index) const = 0;
};

class ExplicitIndexSet : public IndexSet {
  public:
    ExplicitIndexSet(std::set<attr::Index> indices);
    bool contains(attr::Index index) const override;

  private:
    std::set<attr::Index> indices_;
};

class WildcardIndexSet : public IndexSet {
  public:
    bool contains(attr::Index index) const override;
};

} // namespace enzo
