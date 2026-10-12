#pragma once
#include <cstddef>
#include <glm/vec3.hpp>
#include <span>
#include <vector>

namespace enzo::gfx {

/// @brief Returns the indices of the lights closest to a point, in no particular order.
///
/// @param maxCount the most indices returned. Every light is kept when there are no more than this.
std::vector<std::size_t>
getNearestLightIndices(std::span<const glm::vec3> lightPositions, const glm::vec3& point, std::size_t maxCount);

} // namespace enzo::gfx
