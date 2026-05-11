#include "IndexSet.h"

namespace enzo {

ExplicitIndexSet::ExplicitIndexSet(std::set<ga::Index> indices)
    : indices_(std::move(indices)) {}

bool ExplicitIndexSet::contains(ga::Index index) const {
    return indices_.count(index) > 0;
}

bool WildcardIndexSet::contains(ga::Index /*index*/) const {
    return true;
}

} // namespace enzo
