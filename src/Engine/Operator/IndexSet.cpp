#include "Engine/Operator/IndexSet.h"

namespace enzo {

ExplicitIndexSet::ExplicitIndexSet(std::set<Index> indices)
    : indices_(std::move(indices)) {}

bool ExplicitIndexSet::contains(Index index) const {
    return indices_.count(index) > 0;
}

bool WildcardIndexSet::contains(Index /*index*/) const {
    return true;
}

} // namespace enzo
