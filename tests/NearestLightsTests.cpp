#include "Graphics/Passes/NearestLights.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace enzo::gfx;

TEST_CASE("Nearest lights keeps every light when there are no more than the most allowed")
{
    const std::vector<glm::vec3> lightPositions = {{5.f, 0.f, 0.f}, {1.f, 0.f, 0.f}};

    std::vector<std::size_t> lightIndices = getNearestLightIndices(lightPositions, glm::vec3(0.f), 2);
    std::ranges::sort(lightIndices);

    REQUIRE(lightIndices == std::vector<std::size_t>{0, 1});
}

TEST_CASE("Nearest lights keeps the lights closest to the point")
{
    const std::vector<glm::vec3> lightPositions = {{9.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -7.f}, {0.f, 0.f, 2.f}};

    std::vector<std::size_t> lightIndices = getNearestLightIndices(lightPositions, glm::vec3(0.f), 2);
    std::ranges::sort(lightIndices);

    REQUIRE(lightIndices == std::vector<std::size_t>{1, 3});
}
