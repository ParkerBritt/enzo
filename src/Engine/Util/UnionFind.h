#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace enzo::util {

/**
 * @class enzo::util::UnionFind
 * @brief Disjoint-set data structure for tracking connected components.
 *
 * Operates on dense integer IDs from zero to elementCount minus one. Each
 * call to @ref find performs path compression and @ref unite uses union by
 * rank, giving amortized inverse-Ackermann time per operation. Treat as
 * constant time for any practical input size.
 *
 * Typical usage maps domain values (face offsets, point offsets) onto dense
 * IDs and feeds pairs of related IDs into @ref unite. Afterward @ref find
 * reports the root ID of the component containing any given element.
 */
class UnionFind
{
  public:
    /// @brief Constructs an instance with each of @p elementCount elements in its own component.
    explicit UnionFind(std::size_t elementCount);

    /// @brief Returns the root ID of the component containing @p x.
    std::size_t find(std::size_t x);

    /// @brief Joins the components containing @p a and @p b.
    /// @return True when the two were in separate components before the call.
    bool unite(std::size_t a, std::size_t b);

    /// @brief Reports whether @p a and @p b are in the same component.
    bool sameSet(std::size_t a, std::size_t b) { return find(a) == find(b); }

    /// @brief Number of distinct components currently tracked.
    std::size_t componentCount() const { return componentCount_; }

    /// @brief Total elements managed by this instance.
    std::size_t size() const { return parent_.size(); }

  private:
    std::vector<std::size_t> parent_;
    std::vector<std::uint8_t> rank_;
    std::size_t componentCount_;
};

} // namespace enzo::util
