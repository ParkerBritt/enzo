#pragma once

#include "Engine/Core/Types.h"
#include <vector>

namespace enzo::prm {

class Parameter;

/// @brief How a ramp blends from one control point to the next.
// The full set also includes catmull rom, monotone cubic, bezier, and hermite.
// Only constant, linear, and b spline are sampled today.
enum class Interpolation
{
    CONSTANT,
    LINEAR,
    BSPLINE
};

/// @brief A sampled snapshot of a ramp parameter read during cook.
class Ramp
{
  public:
    /// @brief A single ramp control point.
    struct Key
    {
        floatT position;
        floatT value;
        Interpolation interp;
    };

    Ramp() = default;
    explicit Ramp(std::vector<Key> keys);

    /// @brief Snapshots a ramp parameter into a sorted ramp ready for sampling.
    explicit Ramp(const Parameter& rampParameter);

    /// @brief Samples the ramp at @p position.
    /// @return The interpolated value clamped to the end keys outside the domain.
    floatT sample(floatT position) const;

    bool empty() const;
    size_t size() const;
    const Key& key(size_t index) const;

  private:
    /// @brief Samples the segment after @p leftIndex as a uniform cubic b spline.
    floatT sampleBSpline(size_t leftIndex, floatT fraction) const;

    std::vector<Key> keys_;
};

} // namespace enzo::prm
