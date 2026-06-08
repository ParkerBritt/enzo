#include "Engine/Parameter/Ramp.h"
#include "Engine/Parameter/Parameter.h"
#include <algorithm>

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

    // The left key governs how the segment blends toward the right key.
    switch (leftKey.interp)
    {
    case Interpolation::CONSTANT:
        return leftKey.value;
    case Interpolation::LINEAR:
        return leftKey.value + (rightKey.value - leftKey.value) * fraction;
    case Interpolation::BSPLINE:
        return sampleBSpline(leftIndex, fraction);
    }
    return leftKey.value;
}

floatT prm::Ramp::sampleBSpline(size_t leftIndex, floatT fraction) const
{
    // A uniform cubic b spline reads the segment keys plus one neighbour on each
    // side. Missing neighbours at the ends repeat the boundary key, so the curve
    // approaches but does not pass through the end values.
    const size_t lastIndex = keys_.size() - 1;
    const floatT value0 = keys_[leftIndex == 0 ? 0 : leftIndex - 1].value;
    const floatT value1 = keys_[leftIndex].value;
    const floatT value2 = keys_[leftIndex + 1].value;
    const floatT value3 = keys_[std::min(leftIndex + 2, lastIndex)].value;

    const floatT u = fraction;
    const floatT u2 = u * u;
    const floatT u3 = u2 * u;

    // Uniform cubic b spline basis weights, summing to one across the segment.
    const floatT weight0 = (1 - 3 * u + 3 * u2 - u3) / 6;
    const floatT weight1 = (4 - 6 * u2 + 3 * u3) / 6;
    const floatT weight2 = (1 + 3 * u + 3 * u2 - 3 * u3) / 6;
    const floatT weight3 = u3 / 6;

    return weight0 * value0 + weight1 * value1 + weight2 * value2 + weight3 * value3;
}

} // namespace enzo
