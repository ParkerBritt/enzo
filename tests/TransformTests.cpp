#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

namespace {

void requirePointsMatch(const Vector3& point, const Vector3& expected)
{
    REQUIRE(point.x() == Catch::Approx(expected.x()).margin(1e-5));
    REQUIRE(point.y() == Catch::Approx(expected.y()).margin(1e-5));
    REQUIRE(point.z() == Catch::Approx(expected.z()).margin(1e-5));
}

} // namespace

TEST_CASE("Scale Rotate Translate turns a point before moving it")
{
    // Turns the point onto negative Z, then adds the translation.
    const Transform transform = Transform::fromComponents(
        Vector3(0, 0, 5),
        Vector3(0, 90, 0),
        Vector3::Ones(),
        TransformOrder::SRT
    );

    requirePointsMatch(transform * Vector3(1, 0, 0), Vector3(0, 0, 4));
}

TEST_CASE("Translate Rotate Scale moves a point before turning it")
{
    const Transform transform = Transform::fromComponents(
        Vector3(0, 0, 5),
        Vector3(0, 90, 0),
        Vector3::Ones(),
        TransformOrder::TRS
    );

    // Moves the point to (1, 0, 5) first, then the quarter turn swings that
    // whole offset around the origin.
    requirePointsMatch(transform * Vector3(1, 0, 0), Vector3(5, 0, -1));
}

TEST_CASE("Scale Translate Rotate leaves the translation unscaled")
{
    const Transform scaleFirst = Transform::fromComponents(
        Vector3(0, 0, 5),
        Vector3::Zero(),
        Vector3(2, 2, 2),
        TransformOrder::STR
    );
    const Transform scaleLast = Transform::fromComponents(
        Vector3(0, 0, 5),
        Vector3::Zero(),
        Vector3(2, 2, 2),
        TransformOrder::TSR
    );

    requirePointsMatch(scaleFirst * Vector3(1, 0, 0), Vector3(2, 0, 5));
    requirePointsMatch(scaleLast * Vector3(1, 0, 0), Vector3(2, 0, 10));
}

TEST_CASE("An unknown order name falls back to Scale Rotate Translate")
{
    REQUIRE(getTransformOrder("srt") == TransformOrder::SRT);
    REQUIRE(getTransformOrder("trs") == TransformOrder::TRS);
    REQUIRE(getTransformOrder("") == TransformOrder::SRT);
    REQUIRE(getTransformOrder("nonsense") == TransformOrder::SRT);
}

TEST_CASE("The inverse returns a transformed point to where it started")
{
    const Transform transform = Transform::fromComponents(
        Vector3(1, 2, 3),
        Vector3(30, 45, 60),
        Vector3(2, 3, 4),
        TransformOrder::SRT
    );

    const Vector3 point(5, -1, 2);
    requirePointsMatch(transform.getInverse() * (transform * point), point);
}

TEST_CASE("Multiplying two transforms applies the right one first")
{
    const Transform move = Transform().translate(Vector3(5, 0, 0));
    const Transform turn = Transform().rotateEuler(Vector3(0, 90, 0));

    // Moving the origin along X then turning about Y lands it on the negative Z axis
    requirePointsMatch((turn * move) * Vector3(0, 0, 0), Vector3(0, 0, -5));
}
