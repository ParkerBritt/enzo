#include <catch2/catch_test_macros.hpp>
#include <memory>
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Primitives/Mesh.h"


TEST_CASE("attrHandleInt")
{
    using namespace enzo;

    auto myAttrib = std::make_shared<attr::Attribute>("test", attr::AttrType::intT);
    attr::AttributeHandleInt myHandle(myAttrib);
    myHandle.addValue(5);
    myHandle.addValue(6);
    REQUIRE(myHandle.getValue(0)==5);
    REQUIRE(myHandle.getValue(1)==6);
}

TEST_CASE("geometry")
{
    using namespace enzo;
    geo::Mesh geo;
    // check add function
    attr::AttributeHandleInt myHandle = geo.addIntAttribute(attr::AttrOwner::POINT, "index");
    myHandle.addValue(5);
    myHandle.addValue(6);
    REQUIRE(myHandle.getValue(0)==5);
    REQUIRE(myHandle.getValue(1)==6);

    // check getter
    std::shared_ptr<attr::Attribute> myAttribute = geo.getAttribByName(attr::AttrOwner::POINT, "index");
    attr::AttributeHandle<intT> myHandle2(myAttribute);
    REQUIRE(myHandle2.getValue(0)==5);
    REQUIRE(myHandle2.getValue(1)==6);

    // check failed lookup
    std::shared_ptr<attr::Attribute> myAttribute2 = geo.getAttribByName(attr::AttrOwner::POINT, "nonExistant");
    REQUIRE(myAttribute2==nullptr);

}

TEST_CASE("attrHandleFloat")
{
    using namespace enzo;

    std::shared_ptr<attr::Attribute> myAttrib = std::make_shared<attr::Attribute>("test", attr::AttrType::floatT);
    attr::AttributeHandleFloat myHandle(myAttrib);
    myHandle.addValue(5.3f);
    myHandle.addValue(6.9f);
    REQUIRE(myHandle.getValue(0)==5.3f);
    REQUIRE(myHandle.getValue(1)==6.9f);
}

TEST_CASE("attrHandleVector3")
{
    using namespace enzo;

    std::shared_ptr<attr::Attribute> myAttrib = std::make_shared<attr::Attribute>("test", attr::AttrType::vectorT);
    attr::AttributeHandleVector3 myHandle(myAttrib);
    myHandle.addValue(Vector3(5,10,15));
    myHandle.addValue(Vector3(1,2,3));
    REQUIRE(myHandle.getValue(0)==Vector3(5,10,15));
    REQUIRE(myHandle.getValue(1)==Vector3(1,2,3));
}

TEST_CASE("Attribute Type")
{
    using namespace enzo;
    REQUIRE(attr::AttributeType::intT ==      attr::AttributeType::intT);
    REQUIRE(attr::AttributeType::intT !=      attr::AttributeType::floatT);
    REQUIRE(attr::AttributeType::floatT ==    attr::AttributeType::floatT);
    REQUIRE(attr::AttributeType::listT ==     attr::AttributeType::listT);
    REQUIRE(attr::AttributeType::vectorT ==   attr::AttributeType::vectorT);

}

TEST_CASE("Vector3")
{
    using namespace enzo;
    Vector3 u(1,2,3);
    Vector3 v(1,2,3);
    REQUIRE(u == v);
    REQUIRE(u*2 == Vector3(2,4,6));
    REQUIRE(2*u == Vector3(2,4,6));
    REQUIRE(u.x() == 1);
    REQUIRE(u.y() == 2);
    REQUIRE(u.z() == 3);
}

TEST_CASE("Vector4")
{
    using namespace enzo;
    Vector4 u(1,2,3,4);
    REQUIRE(u.x() == 1);
    REQUIRE(u.y() == 2);
    REQUIRE(u.z() == 3);
    REQUIRE(u.w() == 4);
}

