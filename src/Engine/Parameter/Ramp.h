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
    /// @note Reads a dense table baked at construction, so the per point hotpath is a
    /// clamp, an index, and a single blend between the two table samples around it.
    floatT sample(floatT position) const
    {
        if (lookupTable_.empty()) return 0;

        // Map the position onto the table as a fractional step index.
        const floatT tableEndPosition = tableStartPosition_ + tablePositionSpan_;
        const floatT clampedPosition = std::clamp(position, tableStartPosition_, tableEndPosition);
        const floatT stepIndex =
            tablePositionSpan_ > 0
                ? (clampedPosition - tableStartPosition_) / tablePositionSpan_ * lookupTableSteps
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
    /// @brief A curved run of keys precomputed into spline control points.
    /// @note The control arrays already carry the reflected phantom points at both
    /// ends, indexed from one before the first landing key to one past the last, so
    /// sampling reads them straight without rebuilding the run each call.
    struct SplineRun
    {
        int landFirst;
        int landLast;
        std::vector<floatT> controlPositions;
        std::vector<floatT> controlValues;
    };

    /// @brief Groups the curved keys into runs and maps each segment to its run.
    void buildSplineCache();

    /// @brief Bakes the curve into the evenly spaced lookup table sampled at runtime.
    void buildLookupTable();

    /// @brief Evaluates the true ramp curve at @p position, used to fill the table.
    floatT sampleExact(floatT position) const;

    /// @brief Samples @p run at @p position as a parametric uniform cubic b spline.
    /// @note Keys are control points carrying position and value, so a key sways the
    /// curve only within two keys of itself and close keys round rather than pinch.
    floatT sampleBSpline(const SplineRun& run, size_t leftIndex, floatT position) const;

    /// The lookup table holds one more sample than this so both ends are covered.
    static constexpr int lookupTableSteps = 4096;

    std::vector<Key> keys_;

    /// Curved runs and, per segment, the index of the run it curves through or -1.
    std::vector<SplineRun> splineRuns_;
    std::vector<int> segmentRun_;

    /// Evenly spaced samples of the curve across the ramp range, read by sample().
    std::vector<floatT> lookupTable_;
    floatT tableStartPosition_ = 0;
    floatT tablePositionSpan_ = 0;
};

} // namespace enzo::prm
