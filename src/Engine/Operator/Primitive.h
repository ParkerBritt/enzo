#pragma once
#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include "Engine/Operator/AttributeHandle.h"
#include <memory>
#include <string>

namespace enzo::geo
{

/**
* @class enzo::geo::Primitive
* @brief Base class for all primitive types in the engine.
*
* A Primitive is the unit of geometry exchanged between nodes. It holds
* primitive-level attributes (one value per object) and provides the
* attribute management API shared by all primitive types.
*
* Point attributes are stored on the base class since many primitive
* types have points. Use hasPoints() to check before accessing them.
*
* Subclasses (e.g. Mesh) add their own attribute owners and geometry.
*/
class Primitive
{
public:
    using attribVector = std::vector<std::shared_ptr<ga::Attribute>>;

    Primitive();
    Primitive(const Primitive& other);
    Primitive& operator=(const Primitive& rhs);
    virtual ~Primitive() = default;

    virtual PrimType getType() const = 0;
    virtual std::shared_ptr<Primitive> clone() const = 0;
    virtual bool hasPoints() const { return false; }
    virtual ga::Offset getNumPoints() const { return 0; }

    ga::AttributeHandle<bt::intT> addIntAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);
    ga::AttributeHandleBool addBoolAttribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);
    ga::AttributeHandle<bt::Vector3> addVector3Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);
    ga::AttributeHandle<bt::Matrix4> addMatrix4Attribute(ga::AttributeOwner owner, std::string name, bool intrinsic=false);

    std::shared_ptr<ga::Attribute> getAttribByName(ga::AttributeOwner owner, std::string name, bool includeIntrinsics=false);
    const size_t getNumAttributes(const ga::AttributeOwner owner) const;
    std::weak_ptr<const ga::Attribute> getAttributeByIndex(ga::AttributeOwner owner, unsigned int index) const;
    bool attributeExists(ga::AttributeOwner owner, std::string name);

    bt::String getPath() const { return path_; };
    void setPath(const bt::String& path) { path_ = path; };

protected:
    virtual attribVector& getAttributeStore(const ga::AttributeOwner& owner);
    virtual const attribVector& getAttributeStore(const ga::AttributeOwner& owner) const;
    attribVector deepCopyAttributes(attribVector source);

    std::string path_ = "/prim";
    attribVector pointAttributes_;
    attribVector primitiveAttributes_;
};
}
