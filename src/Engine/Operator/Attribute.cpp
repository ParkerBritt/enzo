#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include <memory>
#include <stdexcept>
#include <optional>

using namespace enzo;


ga::Attribute::Attribute(std::string name, ga::AttributeType type)
: name_{name}, type_{type}
{
    // init store
    switch(type_)
    {
        case(AttrType::intT):
            intStore_=std::make_shared<std::vector<bt::intT>>();
            typeSize_=1;
            break;
        case(AttrType::floatT):
            floatStore_=std::make_shared<std::vector<bt::floatT>>();
            typeSize_=1;
            break;
        case(AttrType::vectorT):
            vector3Store_=std::make_shared<std::vector<enzo::bt::Vector3>>();
            typeSize_=3;
            break;
        case(AttrType::boolT):
            boolStore_=std::make_shared<std::vector<enzo::bt::boolT>>();
            typeSize_=1;
            break;
        default:
            throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in Attribute constructor");

    }

}

unsigned int ga::Attribute::Attribute::getTypeSize() const
{
    return typeSize_;
}



ga::Attribute::Attribute(const Attribute& other)
{
    type_ = other.type_;
    private_= other.private_;
    hidden_ = other.hidden_;
    readOnly_ = other.readOnly_;
    name_ = other.name_;
    typeSize_ = other.typeSize_;

    switch(type_)
    {
        case(AttrType::intT):
            intStore_=std::make_shared<std::vector<bt::intT>>(*other.intStore_);
            break;
        case(AttrType::floatT):
            floatStore_=std::make_shared<std::vector<bt::floatT>>(*other.floatStore_);
            break;
        case(AttrType::vectorT):
            vector3Store_=std::make_shared<std::vector<enzo::bt::Vector3>>(*other.vector3Store_);
            break;
        case(AttrType::boolT):
            boolStore_=std::make_shared<std::vector<enzo::bt::boolT>>(*other.boolStore_);
            break;
        default:
            throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in Attribute copy constructor");

    }
}


ga::AttributeType ga::Attribute::getType() const
{
    return type_;
}

std::string ga::Attribute::getName() const
{
    return name_;
}

