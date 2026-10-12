#include "Graphics/Passes/NearestLights.h"

#include <algorithm>
#include <glm/geometric.hpp>
#include <numeric>

namespace enzo::gfx {

std::vector<std::size_t>
getNearestLightIndices(std::span<const glm::vec3> lightPositions, const glm::vec3& point, std::size_t maxCount)
{
    std::vector<std::size_t> lightIndices(lightPositions.size());
    std::iota(lightIndices.begin(), lightIndices.end(), 0);
    if (lightIndices.size() <= maxCount) return lightIndices;

    auto getSquaredDistance = [&](std::size_t lightIndex) {
        const glm::vec3 offset = lightPositions[lightIndex] - point;
        return glm::dot(offset, offset);
    };
    auto isCloser = [&](std::size_t lightIndex, std::size_t otherLightIndex) {
        return getSquaredDistance(lightIndex) < getSquaredDistance(otherLightIndex);
    };
    std::ranges::nth_element(lightIndices, lightIndices.begin() + maxCount, isCloser);
    lightIndices.resize(maxCount);
    return lightIndices;
}

} // namespace enzo::gfx
