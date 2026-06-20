#include "Engine/Parameter/Ramp.h"
#include "Engine/Parameter/Parameter.h"
#include <algorithm>
#include <cmath>

namespace enzo {

// These evaluate the true ramp curve to fill the lookup table. They run once per
// ramp at construction, not on the sampling hotpath, so they favour clarity.
namespace {

using Key = prm::Ramp::Key;
using prm::Interpolation;

constexpr int maxNewtonSteps = 32;
constexpr floatT newtonTolerance = 1e-6f;

// The uniform cubic b spline basis blended over the four control points around a
// point at the local parameter u in [0, 1], and its slope.
floatT cubicValue(floatT u, const floatT control[4])
{
    const floatT u2 = u * u;
    const floatT u3 = u2 * u;
    return ((1 - 3 * u + 3 * u2 - u3) * control[0] + (4 - 6 * u2 + 3 * u3) * control[1] +
            (1 + 3 * u + 3 * u2 - 3 * u3) * control[2] + u3 * control[3]) /
           6;
}

floatT cubicSlope(floatT u, const floatT control[4])
{
    const floatT u2 = u * u;
    return ((-3 + 6 * u - 3 * u2) * control[0] + (-12 * u + 9 * u2) * control[1] +
            (3 + 6 * u - 9 * u2) * control[2] + 3 * u2 * control[3]) /
           6;
}

// Mirrors @p outer across @p anchor, e.g. anchor 5 outer 7 gives 3.
floatT reflect(floatT anchor, floatT outer) { return 2 * anchor - outer; }

// Rounds the @p segment through a run of b spline keys. The run is the contiguous
// block of b spline keys around the segment. The curve lands on the first key of
// the block and on the key just past the last, with a phantom control point
// reflected across each landing so it meets those keys rather than pulling away.
floatT sampleBSpline(const std::vector<Key>& keys, int segment, floatT position)
{
    const int lastKey = static_cast<int>(keys.size()) - 1;

    int runFirst = segment;
    while (runFirst > 0 && keys[runFirst - 1].interp == Interpolation::BSPLINE)
        --runFirst;
    int runLast = segment;
    while (runLast < lastKey && keys[runLast + 1].interp == Interpolation::BSPLINE)
        ++runLast;
    const int landFirst = runFirst;
    const int landLast = runLast < lastKey ? runLast + 1 : runLast;

    // A control point is a landing key, or its reflection when read past either end.
    const auto controlPosition = [&](int index) -> floatT {
        if (index < landFirst)
            return reflect(keys[landFirst].position, keys[2 * landFirst - index].position);
        if (index > landLast)
            return reflect(keys[landLast].position, keys[2 * landLast - index].position);
        return keys[index].position;
    };
    const auto controlValue = [&](int index) -> floatT {
        if (index < landFirst)
            return reflect(keys[landFirst].value, keys[2 * landFirst - index].value);
        if (index > landLast)
            return reflect(keys[landLast].value, keys[2 * landLast - index].value);
        return keys[index].value;
    };
    const auto controlsAround = [&](const auto& control, floatT t, floatT out[4]) {
        const int base = std::clamp(static_cast<int>(std::floor(t)), landFirst, landLast - 1);
        out[0] = control(base - 1);
        out[1] = control(base);
        out[2] = control(base + 1);
        out[3] = control(base + 2);
        return t - base;
    };

    // Position rises monotonically along the parameter t, so a bracketed Newton
    // search inverts the wanted position to a t, then the value is read at that t.
    const floatT span = keys[segment + 1].position - keys[segment].position;
    floatT t = span > 0 ? segment + (position - keys[segment].position) / span : segment;
    floatT lowerT = landFirst;
    floatT upperT = landLast;
    for (int step = 0; step < maxNewtonSteps; ++step)
    {
        floatT control[4];
        const floatT u = controlsAround(controlPosition, t, control);
        const floatT error = cubicValue(u, control) - position;
        if (std::fabs(error) < newtonTolerance) break;

        if (error > 0)
            upperT = t;
        else
            lowerT = t;

        const floatT slope = cubicSlope(u, control);
        const floatT newtonT = slope > 0 ? t - error / slope : t;
        t = newtonT > lowerT && newtonT < upperT ? newtonT : (lowerT + upperT) / 2;
    }

    floatT control[4];
    const floatT u = controlsAround(controlValue, t, control);
    return cubicValue(u, control);
}

// The exact ramp value at @p position, clamped to the end keys outside the range.
floatT curveValue(const std::vector<Key>& keys, floatT position)
{
    if (position <= keys.front().position) return keys.front().value;
    if (position >= keys.back().position) return keys.back().value;

    // The segment is the gap between the left key and the next key along.
    const auto rightIt =
        std::upper_bound(keys.begin(), keys.end(), position, [](floatT target, const Key& key) {
            return target < key.position;
        });
    const int segment = static_cast<int>((rightIt - 1) - keys.begin());
    const Key& left = keys[segment];
    const Key& right = keys[segment + 1];

    if (left.interp == Interpolation::CONSTANT) return left.value;

    // A b spline key only curves when it sits next to another, so a lone one and a
    // linear key both fall through to the straight blend below.
    if (left.interp == Interpolation::BSPLINE)
    {
        const bool nextCurves = right.interp == Interpolation::BSPLINE;
        const bool previousCurves =
            segment > 0 && keys[segment - 1].interp == Interpolation::BSPLINE;
        if (nextCurves || previousCurves) return sampleBSpline(keys, segment, position);
    }

    const floatT span = right.position - left.position;
    const floatT fraction = span > 0 ? (position - left.position) / span : 0;
    return left.value + (right.value - left.value) * fraction;
}

} // namespace

prm::Ramp::Ramp(std::vector<Key> keys) : keys_{std::move(keys)} { bake(); }

prm::Ramp::Ramp(const Parameter& rampParameter)
{
    const unsigned int instanceCount = rampParameter.getInstanceCount();
    keys_.reserve(instanceCount);
    for (unsigned int instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
    {
        const floatT position =
            rampParameter.getInstanceField(instanceIndex, "position")->evalFloat();
        const floatT value = rampParameter.getInstanceField(instanceIndex, "value")->evalFloat();
        Parameter& interpField = *rampParameter.getInstanceField(instanceIndex, "interp");
        const intT interpIndex = interpField.evalInt();
        const intT interpCount = interpField.getTemplate().getNumOptions();

        // The interp dropdown options mirror the Interpolation enum order.
        const Interpolation interp = (interpIndex >= 0 && interpIndex < interpCount)
                                         ? static_cast<Interpolation>(interpIndex)
                                         : Interpolation::LINEAR;
        keys_.push_back({position, value, interp});
    }

    std::sort(keys_.begin(), keys_.end(), [](const Key& firstKey, const Key& secondKey) {
        return firstKey.position < secondKey.position;
    });

    bake();
}

bool prm::Ramp::empty() const { return keys_.empty(); }

size_t prm::Ramp::size() const { return keys_.size(); }

const prm::Ramp::Key& prm::Ramp::key(size_t index) const { return keys_.at(index); }

void prm::Ramp::bake()
{
    lookupTable_.clear();
    if (keys_.empty()) return;

    // The table spans the full ramp range, from the first key to the last.
    tableStartPosition_ = keys_.front().position;
    tablePositionSpan_ = keys_.back().position - tableStartPosition_;

    // Sample the exact curve at every step so runtime sampling only blends.
    const floatT step = tablePositionSpan_ / lookupTableSteps;
    lookupTable_.resize(lookupTableSteps + 1);
    for (int stepIndex = 0; stepIndex <= lookupTableSteps; ++stepIndex)
        lookupTable_[stepIndex] = curveValue(keys_, tableStartPosition_ + stepIndex * step);
}

} // namespace enzo
