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
        const intT interpIndex = rampParameter.getInstanceField(instanceIndex, "interp")->evalInt();

        // The interp dropdown options mirror the Interpolation enum order.
        const Interpolation interp = (interpIndex >= 0 && interpIndex <= 1)
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
    const Key& leftKey = *(rightIt - 1);
    const Key& rightKey = *rightIt;

    const floatT span = rightKey.position - leftKey.position;
    const floatT fraction = span > 0 ? (position - leftKey.position) / span : 0;

    // The left key governs how the segment blends toward the right key.
    switch (leftKey.interp)
    {
    case Interpolation::CONSTANT:
        return leftKey.value;
    case Interpolation::LINEAR:
        return leftKey.value + (rightKey.value - leftKey.value) * fraction;
    }
    return leftKey.value;
}

} // namespace enzo
