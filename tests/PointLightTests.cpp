#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include "Engine/Primitives/PointLight.h"
#include <catch2/catch_test_macros.hpp>

using namespace enzo;

TEST_CASE("A new point light is white with unit intensity at the origin")
{
    const geo::PointLight light;

    REQUIRE(light.getType() == geo::PrimType::POINT_LIGHT);
    REQUIRE(light.getTransform() == Matrix4::Identity());
    REQUIRE(light.getColor() == Vector3::Ones());
    REQUIRE(light.getIntensity() == 1.f);
}

TEST_CASE("Point light settings are stored as intrinsic primitive attributes")
{
    geo::PointLight light;

    REQUIRE(light.getAttribByName(attr::AttributeOwner::PRIMITIVE, "color") == nullptr);
    REQUIRE(light.getAttribByName(attr::AttributeOwner::PRIMITIVE, "transform", true) != nullptr);
    REQUIRE(light.getAttribByName(attr::AttributeOwner::PRIMITIVE, "color", true) != nullptr);
    REQUIRE(light.getAttribByName(attr::AttributeOwner::PRIMITIVE, "intensity", true) != nullptr);
}

TEST_CASE("A cloned point light keeps its settings and edits apart from the original")
{
    geo::PointLight original;
    original.setColor(Vector3(1.f, 0.5f, 0.f));
    original.setIntensity(4.f);

    const auto clone = std::static_pointer_cast<geo::PointLight>(original.clone());
    REQUIRE(clone->getColor() == Vector3(1.f, 0.5f, 0.f));
    REQUIRE(clone->getIntensity() == 4.f);

    clone->setIntensity(2.f);
    REQUIRE(original.getIntensity() == 4.f);
}

TEST_CASE("An assigned point light edits its own settings")
{
    geo::PointLight source;
    source.setIntensity(3.f);
    geo::PointLight destination;

    destination = source;
    destination.setIntensity(5.f);

    REQUIRE(destination.getIntensity() == 5.f);
    REQUIRE(source.getIntensity() == 3.f);
}

TEST_CASE("Transforming a point light moves it only for primitive transforms")
{
    geo::PointLight light;
    const Matrix4 translation = Transform().translate(Vector3(2.f, 0.f, 0.f)).getMatrix();

    light.applyTransform(translation, TransformClass::POINT);
    REQUIRE(light.getTransform() == Matrix4::Identity());

    light.applyTransform(translation, TransformClass::PRIMITIVE);
    REQUIRE(light.getTransform() == translation);
}
