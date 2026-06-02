#include "IndexSet.h"

namespace enzo {

ExplicitIndexSet::ExplicitIndexSet(std::set<attr::Index> indices)
    : indices_(std::move(indices)) {}

bool ExplicitIndexSet::contains(attr::Index index) const {
    return indices_.count(index) > 0;
}

bool WildcardIndexSet::contains(attr::Index /*index*/) const {
    return true;
}

} // namespace enzo
