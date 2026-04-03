#include <catch2/catch_test_macros.hpp>
#include <memory>
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include "Engine/Operator/Primitive.h"
// #include "Engine/Operator/Primitive.h"


TEST_CASE("attrHandleInt")
{
    using namespace enzo;

    auto myAttrib = std::make_shared<ga::Attribute>("test", ga::AttrType::intT);
    ga::AttributeHandleInt myHandle(myAttrib);
    myHandle.addValue(5);
    myHandle.addValue(6);
    REQUIRE(myHandle.getValue(0)==5);
    REQUIRE(myHandle.getValue(1)==6);
}

TEST_CASE("geometry")
{
    using namespace enzo;
    geo::Primitive geo;
    // check add function
    ga::AttributeHandleInt myHandle = geo.addIntAttribute(ga::AttrOwner::POINT, "index");
    myHandle.addValue(5);
    myHandle.addValue(6);
    REQUIRE(myHandle.getValue(0)==5);
    REQUIRE(myHandle.getValue(1)==6);

    // check getter
    std::shared_ptr<ga::Attribute> myAttribute = geo.getAttribByName(ga::AttrOwner::POINT, "index");
    ga::AttributeHandle<bt::intT> myHandle2(myAttribute);
    REQUIRE(myHandle2.getValue(0)==5);
    REQUIRE(myHandle2.getValue(1)==6);

    // check failed lookup
    std::shared_ptr<ga::Attribute> myAttribute2 = geo.getAttribByName(ga::AttrOwner::POINT, "nonExistant");
    REQUIRE(myAttribute2==nullptr);

}

TEST_CASE("attrHandleFloat")
{
    using namespace enzo;

    std::shared_ptr<ga::Attribute> myAttrib = std::make_shared<ga::Attribute>("test", ga::AttrType::floatT);
    ga::AttributeHandleFloat myHandle(myAttrib);
    myHandle.addValue(5.3f);
    myHandle.addValue(6.9f);
    REQUIRE(myHandle.getValue(0)==5.3f);
    REQUIRE(myHandle.getValue(1)==6.9f);
}

TEST_CASE("attrHandleVector3")
{
    using namespace enzo;

    std::shared_ptr<ga::Attribute> myAttrib = std::make_shared<ga::Attribute>("test", ga::AttrType::vectorT);
    ga::AttributeHandleVector3 myHandle(myAttrib);
    myHandle.addValue(bt::Vector3(5,10,15));
    myHandle.addValue(bt::Vector3(1,2,3));
    REQUIRE(myHandle.getValue(0)==bt::Vector3(5,10,15));
    REQUIRE(myHandle.getValue(1)==bt::Vector3(1,2,3));
}

TEST_CASE("Attribute Type")
{
    using namespace enzo;
    REQUIRE(ga::AttributeType::intT ==      ga::AttributeType::intT);
    REQUIRE(ga::AttributeType::intT !=      ga::AttributeType::floatT);
    REQUIRE(ga::AttributeType::floatT ==    ga::AttributeType::floatT);
    REQUIRE(ga::AttributeType::listT ==     ga::AttributeType::listT);
    REQUIRE(ga::AttributeType::vectorT ==   ga::AttributeType::vectorT);

}

TEST_CASE("Vector3")
{
    using namespace enzo;
    bt::Vector3 u(1,2,3);
    bt::Vector3 v(1,2,3);
    REQUIRE(u == v);
    REQUIRE(u*2 == bt::Vector3(2,4,6));
    REQUIRE(2*u == bt::Vector3(2,4,6));
    REQUIRE(u.x() == 1);
    REQUIRE(u.y() == 2);
    REQUIRE(u.z() == 3);
}

TEST_CASE("Vector4")
{
    using namespace enzo;
    bt::Vector4 u(1,2,3,4);
    REQUIRE(u.x() == 1);
    REQUIRE(u.y() == 2);
    REQUIRE(u.z() == 3);
    REQUIRE(u.w() == 4);
}

