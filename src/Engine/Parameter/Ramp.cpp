#include "Engine/Parameter/Ramp.h"
#include "Engine/Parameter/Parameter.h"
#include <algorithm>
#include <cmath>

namespace enzo {

prm::Ramp::Ramp(std::vector<Key> keys) : keys_{std::move(keys)}
{
    buildSplineCache();
    buildLookupTable();
}

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

    buildSplineCache();
    buildLookupTable();
}

bool prm::Ramp::empty() const { return keys_.empty(); }

size_t prm::Ramp::size() const { return keys_.size(); }

const prm::Ramp::Key& prm::Ramp::key(size_t index) const { return keys_.at(index); }

void prm::Ramp::buildLookupTable()
{
    lookupTable_.clear();
    if (keys_.empty()) return;

    // The table spans the full ramp range, from the first key to the last.
    tableStartPosition_ = keys_.front().position;
    tablePositionSpan_ = keys_.back().position - tableStartPosition_;

    // Sample the true curve at every step so runtime sampling only blends.
    const floatT step = tablePositionSpan_ / lookupTableSteps;
    lookupTable_.resize(lookupTableSteps + 1);
    for (int stepIndex = 0; stepIndex <= lookupTableSteps; ++stepIndex)
        lookupTable_[stepIndex] = sampleExact(tableStartPosition_ + stepIndex * step);
}

floatT prm::Ramp::sampleExact(floatT position) const
{
    if (keys_.empty()) return 0;
    if (position <= keys_.front().position) return keys_.front().value;
    if (position >= keys_.back().position) return keys_.back().value;

    // Binary search keeps sampling cheap on the per point hotpath.
    const auto rightIt = std::upper_bound(
        keys_.begin(),
        keys_.end(),
        position,
        [](floatT samplePosition, const Key& key) { return samplePosition < key.position; }
    );
    const size_t leftIndex = static_cast<size_t>((rightIt - 1) - keys_.begin());
    const Key& leftKey = keys_[leftIndex];
    const Key& rightKey = keys_[leftIndex + 1];

    const floatT span = rightKey.position - leftKey.position;
    const floatT fraction = span > 0 ? (position - leftKey.position) / span : 0;
    const floatT linearValue = leftKey.value + (rightKey.value - leftKey.value) * fraction;

    // The left key governs how the segment blends toward the right key.
    switch (leftKey.interp)
    {
    case Interpolation::CONSTANT:
        return leftKey.value;
    case Interpolation::LINEAR:
        return linearValue;
    case Interpolation::BSPLINE:
    {
        // A run of b spline keys rounds toward the keys bordering it. A lone b spline
        // key has nothing to round against and stays a straight line, so it maps to
        // no run and falls back to the linear blend.
        const int runIndex = segmentRun_[leftIndex];
        if (runIndex >= 0) return sampleBSpline(splineRuns_[runIndex], leftIndex, position);
        return linearValue;
    }
    }
    return leftKey.value;
}

void prm::Ramp::buildSplineCache()
{
    splineRuns_.clear();
    segmentRun_.assign(keys_.size(), -1);
    const int lastIndex = static_cast<int>(keys_.size()) - 1;

    // Walk the keys collecting each maximal block of b spline keys.
    int keyIndex = 0;
    while (keyIndex <= lastIndex)
    {
        if (keys_[keyIndex].interp != Interpolation::BSPLINE)
        {
            ++keyIndex;
            continue;
        }
        int blockFirst = keyIndex;
        int blockLast = keyIndex;
        while (blockLast < lastIndex && keys_[blockLast + 1].interp == Interpolation::BSPLINE)
            ++blockLast;
        keyIndex = blockLast + 1;

        // A lone b spline key has nothing to round against and curves nothing.
        if (blockLast == blockFirst) continue;

        // The curve lands on the first key of the block and on the key one step past
        // the last, clamped to the ramp end.
        SplineRun run;
        run.landFirst = blockFirst;
        run.landLast = blockLast < lastIndex ? blockLast + 1 : blockLast;
        const int controlCount = run.landLast - run.landFirst + 3;
        run.controlPositions.resize(controlCount);
        run.controlValues.resize(controlCount);

        // Reflect a phantom control point across each landing key so the curve meets
        // that key while the interior keys stay rounded. The real keys fill the
        // middle, indexed one past the low phantom.
        run.controlPositions.front() =
            2 * keys_[run.landFirst].position - keys_[run.landFirst + 1].position;
        run.controlValues.front() =
            2 * keys_[run.landFirst].value - keys_[run.landFirst + 1].value;
        for (int landIndex = run.landFirst; landIndex <= run.landLast; ++landIndex)
        {
            run.controlPositions[landIndex - run.landFirst + 1] = keys_[landIndex].position;
            run.controlValues[landIndex - run.landFirst + 1] = keys_[landIndex].value;
        }
        run.controlPositions.back() =
            2 * keys_[run.landLast].position - keys_[run.landLast - 1].position;
        run.controlValues.back() =
            2 * keys_[run.landLast].value - keys_[run.landLast - 1].value;

        // Every segment whose left key sits in the block curves through this run.
        const int runIndex = static_cast<int>(splineRuns_.size());
        const int lastSegment = std::min(blockLast, lastIndex - 1);
        for (int segment = blockFirst; segment <= lastSegment; ++segment)
            segmentRun_[segment] = runIndex;

        splineRuns_.push_back(std::move(run));
    }
}

floatT prm::Ramp::sampleBSpline(const SplineRun& run, size_t leftIndex, floatT position) const
{
    // The curved keys form a parametric uniform cubic b spline. Each key is a
    // control point carrying its position and value, so sampling finds the
    // parameter where the spline reaches the wanted position and reads its value
    // there. Uniform knots give every key the same local pull, so a key only sways
    // the curve within two keys of itself and close keys round rather than pinch.
    const int landFirst = run.landFirst;
    const int landLast = run.landLast;
    const int segment = static_cast<int>(leftIndex);
    const floatT* const positions = run.controlPositions.data();
    const floatT* const values = run.controlValues.data();

    // Read the four control points around parameter t into c0 to c3. The integer
    // part of t selects the segment and indexes straight into the run arrays.
    const auto controlsAt = [&](const floatT* control, floatT t, floatT* out) {
        int base = static_cast<int>(std::floor(t));
        base = std::min(std::max(base, landFirst), landLast - 1);
        const int localBase = base - landFirst;
        out[0] = control[localBase];
        out[1] = control[localBase + 1];
        out[2] = control[localBase + 2];
        out[3] = control[localBase + 3];
        return t - base;
    };

    // The uniform cubic basis blended over the four control points.
    const auto valueOf = [](floatT u, const floatT* control) -> floatT {
        const floatT u2 = u * u;
        const floatT u3 = u2 * u;
        return ((1 - 3 * u + 3 * u2 - u3) * control[0] + (4 - 6 * u2 + 3 * u3) * control[1] +
                (1 + 3 * u + 3 * u2 - 3 * u3) * control[2] + u3 * control[3]) /
               6;
    };
    // Its derivative, used by the Newton step on the position spline.
    const auto slopeOf = [](floatT u, const floatT* control) -> floatT {
        const floatT u2 = u * u;
        return ((-3 + 6 * u - 3 * u2) * control[0] + (-12 * u + 9 * u2) * control[1] +
                (3 + 6 * u - 9 * u2) * control[2] + 3 * u2 * control[3]) /
               6;
    };

    // The spline position rises monotonically along the run, so a bracketed Newton
    // search inverts position to a parameter quickly and never escapes the run.
    floatT lowerT = landFirst;
    floatT upperT = landLast;
    const floatT span = keys_[segment + 1].position - keys_[segment].position;
    floatT t = span > 0 ? segment + (position - keys_[segment].position) / span : segment;
    for (int step = 0; step < 32; ++step)
    {
        floatT control[4];
        const floatT u = controlsAt(positions, t, control);
        const floatT offset = valueOf(u, control) - position;
        if (std::fabs(offset) < 1e-6f) break;
        if (offset > 0)
            upperT = t;
        else
            lowerT = t;
        const floatT positionSlope = slopeOf(u, control);
        const floatT newtonT = positionSlope > 0 ? t - offset / positionSlope : t;
        t = newtonT > lowerT && newtonT < upperT ? newtonT : (lowerT + upperT) / 2;
    }

    floatT control[4];
    const floatT u = controlsAt(values, t, control);
    return valueOf(u, control);
}

} // namespace enzo
