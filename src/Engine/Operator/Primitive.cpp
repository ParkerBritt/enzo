#include "Engine/Operator/Primitive.h"
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Types.h"
#include <memory>
#include <stdexcept>
#include <string>

using namespace enzo;

geo::Primitive::Primitive()
{
}

geo::Primitive::Primitive(const Primitive& other):
    pointAttributes_{deepCopyAttributes(other.pointAttributes_)},
    primitiveAttributes_{deepCopyAttributes(other.primitiveAttributes_)},
    path_{other.path_}
{
}

enzo::geo::Primitive& enzo::geo::Primitive::operator=(const enzo::geo::Primitive& rhs) {
    if (this == &rhs) return *this;

    pointAttributes_      = deepCopyAttributes(rhs.pointAttributes_);
    primitiveAttributes_  = deepCopyAttributes(rhs.primitiveAttributes_);
    path_                 = rhs.path_;

    return *this;
}

const size_t geo::Primitive::getNumAttributes(const ga::AttributeOwner owner) const
{
    return getAttributeStore(owner).size();
}

std::weak_ptr<const ga::Attribute> geo::Primitive::getAttributeByIndex(ga::AttributeOwner owner, unsigned int index) const
{
    auto attribStore = getAttributeStore(owner);
    if(index>=attribStore.size())
    {
        throw std::out_of_range("Attribute index out of range: " + std::to_string(index) + " max size: " + std::to_string(attribStore.size()) + "\n");
    }
    return getAttributeStore(owner)[index];
}

ga::AttributeHandleInt geo::Primitive::addIntAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::intT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleInt(newAttribute);
}

ga::AttributeHandleBool geo::Primitive::addBoolAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::boolT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandleBool(newAttribute);
}

ga::AttributeHandle<bt::Vector3> geo::Primitive::addVector3Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic)
{
    auto newAttribute = std::make_shared<ga::Attribute>(name, ga::AttrType::vectorT, intrinsic);
    getAttributeStore(owner).push_back(newAttribute);
    return ga::AttributeHandle<bt::Vector3>(newAttribute);
}

geo::Primitive::attribVector& geo::Primitive::getAttributeStore(const ga::AttributeOwner& owner)
{
    switch(owner)
    {
        case ga::AttributeOwner::POINT:
            return pointAttributes_;
        case ga::AttributeOwner::PRIMITIVE:
            return primitiveAttributes_;
        default:
            throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

const geo::Primitive::attribVector& geo::Primitive::getAttributeStore(const ga::AttributeOwner& owner) const
{
    switch(owner)
    {
        case ga::AttributeOwner::POINT:
            return pointAttributes_;
        case ga::AttributeOwner::PRIMITIVE:
            return primitiveAttributes_;
        default:
            throw std::runtime_error("AttributeOwner not supported by this primitive type");
    }
}

bool geo::Primitive::attributeExists(ga::AttributeOwner owner, std::string name)
{
    return static_cast<bool>(getAttribByName(owner, name));
}

std::shared_ptr<ga::Attribute> geo::Primitive::getAttribByName(ga::AttributeOwner owner, std::string name, bool includeIntrinsics)
{
    auto& vector = getAttributeStore(owner);
    for(auto it=vector.begin(); it!=vector.end(); ++it)
    {
        std::shared_ptr<ga::Attribute> attribute = (*it);
        if(attribute->getName()==name)
        {
            if(!includeIntrinsics && attribute->isIntrinsic()) continue;
            return attribute;
        }
    }
    return nullptr;
}

geo::Primitive::attribVector geo::Primitive::deepCopyAttributes(attribVector originalVector)
{
    geo::Primitive::attribVector copied;
    const size_t sourceSize = originalVector.size();

    copied.reserve(sourceSize);

    for(const std::shared_ptr<ga::Attribute> sourceAttrib : originalVector)
    {
        if(sourceAttrib)
        {
            copied.push_back(std::make_shared<ga::Attribute>(*sourceAttrib));
        }
        else
        {
            copied.push_back(nullptr);
        }
    }

    return copied;
}
