#include "Engine/Util/UnionFind.h"
#include <utility>

namespace enzo::util
{

UnionFind::UnionFind(std::size_t elementCount)
    : parent_(elementCount), rank_(elementCount, 0), componentCount_(elementCount)
{
    for (std::size_t i = 0; i < elementCount; ++i) parent_[i] = i;
}

std::size_t UnionFind::find(std::size_t x)
{
    while (parent_[x] != x)
    {
        parent_[x] = parent_[parent_[x]];
        x = parent_[x];
    }
    return x;
}

bool UnionFind::unite(std::size_t a, std::size_t b)
{
    a = find(a);
    b = find(b);
    if (a == b) return false;

    // Attach the shallower tree under the deeper one
    if (rank_[a] < rank_[b]) std::swap(a, b);
    parent_[b] = a;
    if (rank_[a] == rank_[b]) ++rank_[a];

    --componentCount_;
    return true;
}

} // namespace util
