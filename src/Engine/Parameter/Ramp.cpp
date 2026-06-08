#include "Engine/Parameter/Ramp.h"
#include "Engine/Parameter/Parameter.h"
#include <algorithm>
#include <cmath>

namespace enzo {

prm::Ramp::Ramp(std::vector<Key> keys) : keys_{std::move(keys)} {}

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
}

bool prm::Ramp::empty() const { return keys_.empty(); }

size_t prm::Ramp::size() const { return keys_.size(); }

const prm::Ramp::Key& prm::Ramp::key(size_t index) const { return keys_.at(index); }

floatT prm::Ramp::sample(floatT position) const
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
        // A run of b spline keys rounds toward the keys bordering it. A segment
        // curves when either its right key or its left neighbour is a b spline, so
        // the run reaches one key past its last b spline key. A lone b spline key
        // has nothing to round against and stays a straight line.
        const bool nextCurves = rightKey.interp == Interpolation::BSPLINE;
        const bool previousCurves =
            leftIndex > 0 && keys_[leftIndex - 1].interp == Interpolation::BSPLINE;
        if (nextCurves || previousCurves) return sampleBSpline(leftIndex, position);
        return linearValue;
    }
    }
    return leftKey.value;
}

floatT prm::Ramp::sampleBSpline(size_t leftIndex, floatT position) const
{
    // The curved keys form a parametric uniform cubic b spline. Each key is a
    // control point carrying its position and value, so sampling finds the
    // parameter where the spline reaches the wanted position and reads its value
    // there. Uniform knots give every key the same local pull, so a key only sways
    // the curve within two keys of itself and close keys round rather than pinch.
    const int lastIndex = static_cast<int>(keys_.size()) - 1;
    const int segment = static_cast<int>(leftIndex);

    // The run is the contiguous block of b spline keys around this segment. The
    // curve lands on the first b spline key and on the key one step past the last,
    // clamped to the ramp ends.
    int runFirst = segment;
    while (runFirst > 0 && keys_[runFirst - 1].interp == Interpolation::BSPLINE) --runFirst;
    int runLast = segment;
    while (runLast < lastIndex && keys_[runLast + 1].interp == Interpolation::BSPLINE) ++runLast;
    const int landFirst = runFirst;
    const int landLast = runLast < lastIndex ? runLast + 1 : runLast;

    // Reflect a phantom control point across a landing key so the curve meets that
    // key while the interior keys stay rounded.
    const auto controlPosition = [&](int keyIndex) -> floatT {
        if (keyIndex < landFirst)
            return 2 * keys_[landFirst].position - keys_[2 * landFirst - keyIndex].position;
        if (keyIndex > landLast)
            return 2 * keys_[landLast].position - keys_[2 * landLast - keyIndex].position;
        return keys_[keyIndex].position;
    };
    const auto controlValue = [&](int keyIndex) -> floatT {
        if (keyIndex < landFirst)
            return 2 * keys_[landFirst].value - keys_[2 * landFirst - keyIndex].value;
        if (keyIndex > landLast)
            return 2 * keys_[landLast].value - keys_[2 * landLast - keyIndex].value;
        return keys_[keyIndex].value;
    };

    // Evaluate a spline component at parameter t. The integer part selects the
    // segment and the fraction drives the uniform cubic basis or its slope.
    const auto evaluate = [&](const auto& control, floatT t, bool slope) -> floatT {
        int base = static_cast<int>(std::floor(t));
        base = std::min(std::max(base, landFirst), landLast - 1);
        const floatT u = t - base;
        const floatT u2 = u * u;
        const floatT control0 = control(base - 1);
        const floatT control1 = control(base);
        const floatT control2 = control(base + 1);
        const floatT control3 = control(base + 2);
        if (slope)
        {
            const floatT weight0 = (-3 + 6 * u - 3 * u2) / 6;
            const floatT weight1 = (-12 * u + 9 * u2) / 6;
            const floatT weight2 = (3 + 6 * u - 9 * u2) / 6;
            const floatT weight3 = 3 * u2 / 6;
            return weight0 * control0 + weight1 * control1 + weight2 * control2 +
                   weight3 * control3;
        }
        const floatT u3 = u2 * u;
        const floatT weight0 = (1 - 3 * u + 3 * u2 - u3) / 6;
        const floatT weight1 = (4 - 6 * u2 + 3 * u3) / 6;
        const floatT weight2 = (1 + 3 * u + 3 * u2 - 3 * u3) / 6;
        const floatT weight3 = u3 / 6;
        return weight0 * control0 + weight1 * control1 + weight2 * control2 + weight3 * control3;
    };

    // The spline position rises monotonically along the run, so a bracketed Newton
    // search inverts position to a parameter quickly and never escapes the run.
    floatT lowerT = landFirst;
    floatT upperT = landLast;
    const floatT span = keys_[segment + 1].position - keys_[segment].position;
    floatT t = span > 0 ? segment + (position - keys_[segment].position) / span : segment;
    for (int step = 0; step < 32; ++step)
    {
        const floatT offset = evaluate(controlPosition, t, false) - position;
        if (std::fabs(offset) < 1e-6f) break;
        if (offset > 0)
            upperT = t;
        else
            lowerT = t;
        const floatT positionSlope = evaluate(controlPosition, t, true);
        const floatT newtonT = positionSlope > 0 ? t - offset / positionSlope : t;
        t = newtonT > lowerT && newtonT < upperT ? newtonT : (lowerT + upperT) / 2;
    }
    return evaluate(controlValue, t, false);
}

} // namespace enzo
