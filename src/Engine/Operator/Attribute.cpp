#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include <memory>
#include <stdexcept>
#include <optional>

using namespace enzo;


ga::Attribute::Attribute(std::string name, ga::AttributeType type, bool intrinsic)
: name_{name}, type_{type}, intrinsic_{intrinsic}
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
        case(AttrType::matrixT):
            matrix4Store_=std::make_shared<std::vector<enzo::bt::Matrix4>>();
            typeSize_=16;
            break;
        default:
            throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in Attribute constructor");

    }

}

unsigned int ga::Attribute::Attribute::getTypeSize() const
{
    return typeSize_;
}


void ga::Attribute::resize(size_t size)
{
    using namespace enzo::ga;
    switch(type_)
    {
        case AttributeType::intT:
            intStore_->resize(size);
            break;
        case AttributeType::floatT:
            floatStore_->resize(size);
            break;
        case AttributeType::boolT:
            boolStore_->resize(size);
            break;
        case AttributeType::vectorT:
            vector3Store_->resize(size);
            break;
        case AttributeType::matrixT:
            matrix4Store_->resize(size);
            break;
        default:
            throw std::runtime_error("type not accounted for");

    }
    
}

bool ga::Attribute::isIntrinsic() const
{
    return intrinsic_;
}



ga::Attribute::Attribute(const Attribute& other)
{
    type_ = other.type_;
    // private_= other.private_;
    // hidden_ = other.hidden_;
    // readOnly_ = other.readOnly_;
    intrinsic_ = other.intrinsic_;
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
        case(AttrType::matrixT):
            matrix4Store_=std::make_shared<std::vector<enzo::bt::Matrix4>>(*other.matrix4Store_);
            break;
        default:
            throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in Attribute copy constructor");

    }
}


bt::Vector3 ga::Attribute::getVector3(ga::Offset offset) const
{
    return (*vector3Store_)[offset];
}

bt::Matrix4 ga::Attribute::getMatrix4(ga::Offset offset) const
{
    return (*matrix4Store_)[offset];
}

size_t ga::Attribute::getSize() const
{
    switch(type_)
    {
        case AttrType::intT:    return intStore_->size();
        case AttrType::floatT:  return floatStore_->size();
        case AttrType::vectorT: return vector3Store_->size();
        case AttrType::boolT:   return boolStore_->size();
        case AttrType::matrixT: return matrix4Store_->size();
        default: return 0;
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

