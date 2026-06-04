#pragma once
#include "Engine/Core/Types.h"
#include <set>

namespace enzo {

class IndexSet
{
  public:
    virtual ~IndexSet() = default;
    virtual bool contains(Index index) const = 0;
};

class ExplicitIndexSet : public IndexSet
{
  public:
    ExplicitIndexSet(std::set<Index> indices);
    bool contains(Index index) const override;

  private:
    std::set<Index> indices_;
};

class WildcardIndexSet : public IndexSet
{
  public:
    bool contains(Index index) const override;
};

} // namespace enzo
