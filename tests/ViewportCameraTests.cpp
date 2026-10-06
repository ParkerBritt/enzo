#include "Graphics/ViewportCamera.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

using namespace enzo::gfx;
using Catch::Approx;

namespace {

glm::vec3 toViewSpace(const ViewportCamera& camera, glm::vec3 worldPoint)
{
    return glm::vec3(camera.getViewMatrix() * glm::vec4(worldPoint, 1.f));
}

} // namespace

TEST_CASE("A viewport camera starts looking at the origin")
{
    ViewportCamera camera;
    const glm::vec3 origin = toViewSpace(camera, glm::vec3(0.f));

    REQUIRE(origin.x == Approx(0.f).margin(1e-5));
    REQUIRE(origin.y == Approx(0.f).margin(1e-5));
    REQUIRE(origin.z < 0.f);
}

TEST_CASE("A viewport camera keeps its distance from the centre while orbiting")
{
    ViewportCamera camera;
    const float distanceBefore = glm::length(camera.getPosition());

    camera.orbit({120.f, -40.f});

    REQUIRE(glm::length(camera.getPosition()) == Approx(distanceBefore));
    REQUIRE(toViewSpace(camera, glm::vec3(0.f)).x == Approx(0.f).margin(1e-5));
}

TEST_CASE("A viewport camera stops short of looking straight down")
{
    ViewportCamera camera;
    camera.orbit({0.f, 100000.f});

    const glm::vec3 position = camera.getPosition();
    const float horizontalDistance = glm::length(glm::vec2(position.x, position.z));
    REQUIRE(position.y > 0.f);
    REQUIRE(horizontalDistance > 0.01f);
}

TEST_CASE("A viewport camera moves the scene with the pointer when panning")
{
    ViewportCamera camera;
    const glm::vec3 originBefore = toViewSpace(camera, glm::vec3(0.f));

    // Drags right and down, in pixels with y growing downward.
    camera.pan({50.f, 30.f});
    const glm::vec3 originAfter = toViewSpace(camera, glm::vec3(0.f));

    REQUIRE(originAfter.x > originBefore.x);
    REQUIRE(originAfter.y < originBefore.y);
    REQUIRE(originAfter.z == Approx(originBefore.z));
}

TEST_CASE("A viewport camera dollies toward the centre")
{
    ViewportCamera camera;
    const float distanceBefore = glm::length(camera.getPosition());

    camera.dolly(-1.f);

    REQUIRE(glm::length(camera.getPosition()) < distanceBefore);
    REQUIRE(toViewSpace(camera, glm::vec3(0.f)).x == Approx(0.f).margin(1e-5));
}

TEST_CASE("A viewport camera carries the centre forward when dollying past it")
{
    ViewportCamera camera;
    const glm::vec3 start = camera.getPosition();
    const glm::vec3 forward = -glm::normalize(start);

    camera.dolly(-1000.f);
    const glm::vec3 position = camera.getPosition();

    // Travels along the original view line past the old centre at the origin.
    REQUIRE(glm::dot(position - start, forward) > glm::length(start));
    REQUIRE(glm::length(glm::cross(position - start, forward)) == Approx(0.f).margin(1e-3));

    // Keeps the origin behind the camera after passing it.
    REQUIRE(toViewSpace(camera, glm::vec3(0.f)).z > 0.f);
}

TEST_CASE("A viewport camera projects depth from 0 at the near plane to 1 at the far plane")
{
    ViewportCamera camera;
    const glm::mat4 projection = camera.getProjectionMatrix(1.f);

    const glm::vec4 nearPoint = projection * glm::vec4(0.f, 0.f, -0.1f, 1.f);
    const glm::vec4 farPoint = projection * glm::vec4(0.f, 0.f, -1000.f, 1.f);

    REQUIRE(nearPoint.z / nearPoint.w == Approx(0.f).margin(1e-5));
    REQUIRE(farPoint.z / farPoint.w == Approx(1.f));
}
