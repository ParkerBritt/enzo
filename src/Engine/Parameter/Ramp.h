#pragma once

#include "Engine/Core/Types.h"
#include <algorithm>
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
/// @note Construction bakes the curve into a dense lookup table, so sampling on the
/// per point hotpath is just a clamp, an index, and a single blend between entries.
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
    /// @return The interpolated value clamped to the end keys outside the ramp range.
    floatT sample(floatT position) const
    {
        if (lookupTable_.empty()) return 0;

        // Map the position onto the table as a fractional step index.
        const floatT tableEndPosition = tableStartPosition_ + tablePositionSpan_;
        const floatT clampedPosition = std::clamp(position, tableStartPosition_, tableEndPosition);
        const floatT stepIndex = tablePositionSpan_ > 0 ? (clampedPosition - tableStartPosition_) /
                                                              tablePositionSpan_ * lookupTableSteps
                                                        : 0;

        // Blend the two table samples bracketing that step.
        int lowerStep = static_cast<int>(stepIndex);
        if (lowerStep >= lookupTableSteps) lowerStep = lookupTableSteps - 1;
        const floatT blend = stepIndex - static_cast<floatT>(lowerStep);
        return lookupTable_[lowerStep] +
               (lookupTable_[lowerStep + 1] - lookupTable_[lowerStep]) * blend;
    }

    bool empty() const;
    size_t size() const;
    const Key& key(size_t index) const;

  private:
    /// @brief Bakes the curve into the evenly spaced lookup table read by sample().
    void bake();

    /// The lookup table holds one more sample than this so both ramp ends are covered.
    static constexpr int lookupTableSteps = 4096;

    std::vector<Key> keys_;
    std::vector<floatT> lookupTable_;
    floatT tableStartPosition_ = 0;
    floatT tablePositionSpan_ = 0;
};

} // namespace enzo::prm
