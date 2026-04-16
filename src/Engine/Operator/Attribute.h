#pragma once
#include "Engine/Types.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace enzo {
namespace ga {
template <typename T> using StoreContainer = std::vector<T>;

template <typename T> class AttributeHandle;

/**
 * @class enzo::ga::Attribute
 * @brief Containers for geometry data.
 *
 * Attributes are the containers for geometry data in the engine.
 * Each attribute is a single column in the spreadsheet that makes up your geometry.
 * The column has a single name and type. Based on the attribute owner it will also
 * have a value for each element. For example, if the owner is a point then your
 * attribute might be Color, your type might be a vector, and the attribute will
 * have a color value for each point in the mesh. Attributes own the underlying
 * storage for their values, which is contiguous and strongly typed
 * (e.g., ints, floats, vectors, bools). This allows a single attribute class
 * to store data regardless of the type without templates.
 * Attributes cannot be read or written from by themselves.
 * For that an attribute handle is required, (see #enzo::ga::AttributeHandle).
 *
 * @note Values are strongly typed; invalid type combinations are guarded both
 *       at compile time (via handles) and at runtime (defensive checks).
 */
class Attribute {
  public:
    /**
     * @brief Construct a new attribute and initialize its typed storage.
     *
     * @param name Human-readable identifier, spaces are not permitted (unique within a
     * collection/scope).
     * @param type Attribute data type that values will be stored in.
     *
     */
    Attribute(std::string name, ga::AttributeType type, bool intrinsic = false);
    Attribute(const Attribute &other);
    /**
     * @brief Returns the attribute type this attribute stores.
     */
    AttributeType getType() const;
    /**
     * @brief Returns the name of this attribute.
     */
    std::string getName() const;

    bt::Vector3 getVector3(Offset offset) const;
    bt::Matrix4 getMatrix4(Offset offset) const;
    size_t getSize() const;
    /**
     * @brief Returns the number of components in the type (eg. StringT is 1, vectorT is 3).
     */
    unsigned int getTypeSize() const;

    /**
     * @brief Returns whether the attribute is intrinsic
     *
     * An intrinsic attribute is an inherent property of the geometry created with the geometry
     * automatically and cannot be deleted.
     */
    bool isIntrinsic() const;

    /**
     *  @brief Changes the number of elements stored
     */
    void resize(size_t size);

    template <typename T> friend class AttributeHandle;
    template <typename T> friend class AttributeHandleRO;

  private:
    // private attributes are attributes that are hidden from the user
    // for internal use
    // bool private_=false;
    // hidden attributes are user accessible attributes that the user may
    // or may want to use
    // bool hidden_=false;
    // allows the user to read the attribute but not modify it
    // bool readOnly_=false;
    bool intrinsic_ = false;

    ga::AttributeType type_;
    unsigned int typeSize_ = 1;

    std::string name_;

    // void* data_;

    // data stores
    std::shared_ptr<StoreContainer<bt::intT>> intStore_;
    std::shared_ptr<StoreContainer<bt::floatT>> floatStore_;
    std::shared_ptr<StoreContainer<enzo::bt::Vector3>> vector3Store_;
    std::shared_ptr<StoreContainer<enzo::bt::boolT>> boolStore_;
    std::shared_ptr<StoreContainer<enzo::bt::Matrix4>> matrix4Store_;
};

using attribVector = std::vector<std::shared_ptr<ga::Attribute>>;
} // namespace ga
} // namespace enzo
