#pragma once
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Attribute/Point.h"
#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include <memory>
#include <string>

namespace enzo::geo {

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
class Primitive {
  public:
    // Point Iterator
    struct PointOffsets {
        PointOffsets(Primitive &primitive) : primitive_(primitive) {}
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Offset;

            value_type operator*() const { return curOffset_; }

            Iterator(Primitive &primitive, Offset current)
                : primitive_(primitive), curOffset_(current) {
                skipInvalid();
            }

            // Prefix increment
            Iterator &operator++() {
                ++curOffset_;
                skipInvalid();
                return *this;
            }

            // Postfix increment
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator &a, const Iterator &b) {
                return a.curOffset_ == b.curOffset_;
            };
            friend bool operator!=(const Iterator &a, const Iterator &b) {
                return a.curOffset_ != b.curOffset_;
            };

          private:
            enzo::geo::Primitive &primitive_;
            Offset curOffset_ = 0;
            void skipInvalid() {
                const Offset end = primitive_.getNumPoints();
                while (curOffset_ < end && !primitive_.isValidPoint(curOffset_)) ++curOffset_;
            }
        };
        Iterator begin() { return Iterator(primitive_, 0); }
        Iterator end() { return Iterator(primitive_, primitive_.getNumPoints()); }

      private:
        enzo::geo::Primitive &primitive_;
    };
    Primitive(std::string_view path="/prim");
    Primitive(const Primitive &other);
    Primitive &operator=(const Primitive &rhs);
    virtual ~Primitive() = default;

    virtual PrimType getType() const = 0;
    virtual std::shared_ptr<Primitive> clone() const = 0;
    virtual TransformClass transformType() const = 0;
    virtual void applyTransform(const Matrix4 &mat, TransformClass transformClass) = 0;
    void applyTransform(const Transform &transform, TransformClass transformClass) {
        applyTransform(transform.getMatrix(), transformClass);
    }
    virtual bool canMerge() const { return false; }
    virtual void merge(std::shared_ptr<Primitive> other) {}
    void incrementVersion();

    virtual bool hasPoints() const { return false; }
    Offset getNumPoints() const { return getElementCount(attr::AttributeOwner::POINT); }
    virtual PointOffsets getPoints() { return PointOffsets(*this); }
    virtual bool isValidPoint(Offset offset) const { return true; }
    virtual void deletePoints(const std::vector<Offset>& pointOffsets) {}

    /**
     * @brief Compacts storage, removing entries marked invalid so offsets
     * are contiguous again.
     */
    virtual void defragment() {}

    attr::AttributeHandle<intT> addIntAttribute(attr::AttributeOwner owner, std::string name,
                                                  bool intrinsic = false);
    attr::AttributeHandleBool addBoolAttribute(attr::AttributeOwner owner, std::string name,
                                             bool intrinsic = false, bool isPrivate = false);
    attr::AttributeHandle<Vector3> addVector3Attribute(attr::AttributeOwner owner, std::string name,
                                                         bool intrinsic = false);
    attr::AttributeHandle<Matrix4> addMatrix4Attribute(attr::AttributeOwner owner, std::string name,
                                                         bool intrinsic = false);

    std::shared_ptr<attr::Attribute> getAttribByName(attr::AttributeOwner owner, std::string name,
                                                   bool includeIntrinsics = false);
    /**
     * @brief Const counterpart of @ref getAttribByName.
     *
     * Returns a read only shared pointer so the caller cannot mutate the
     * attribute through a const Primitive.
     */
    std::shared_ptr<const attr::Attribute> getAttribByName(attr::AttributeOwner owner,
                                                         std::string name,
                                                         bool includeIntrinsics = false) const;
    const size_t getNumAttributes(const attr::AttributeOwner owner) const;
    std::weak_ptr<const attr::Attribute> getAttributeByIndex(attr::AttributeOwner owner,
                                                           unsigned int index) const;
    bool attributeExists(attr::AttributeOwner owner, std::string name);

    /**
     * @brief Creates a group on the given owner.
     *
     * Groups are boolean flags that mark elements as members. A group
     * and a regular attribute can share a name without colliding.
     *
     * @return Handle to the new group.
     */
    attr::AttributeHandleBool createGroup(attr::AttributeOwner owner, std::string name);
    /**
     * @brief Marks the given offsets as members of the group.
     */
    void addToGroup(attr::AttributeOwner owner, const std::string& name,
                    const std::vector<Offset>& offsets);
    /**
     * @brief Looks up a group by name.
     * @return The matching group, or nullptr if no such group exists.
     */
    std::shared_ptr<attr::Attribute> getGroupByName(attr::AttributeOwner owner,
                                                  const std::string& name) const;
    /**
     * @brief Returns how many groups live on the given owner.
     */
    size_t getNumGroups(attr::AttributeOwner owner) const;
    /**
     * @brief Returns the group at the given index in the owner's group store.
     */
    std::weak_ptr<const attr::Attribute> getGroupByIndex(attr::AttributeOwner owner,
                                                       unsigned int index) const;

    /// @brief Creates a point group.
    /// @return Handle to the new group.
    attr::AttributeHandleBool createPointGroup(std::string name) {
        return createGroup(attr::AttributeOwner::POINT, std::move(name));
    }
    /// @brief Creates a primitive group.
    /// @return Handle to the new group.
    attr::AttributeHandleBool createPrimitiveGroup(std::string name) {
        return createGroup(attr::AttributeOwner::PRIMITIVE, std::move(name));
    }
    /// @brief Marks the given offsets as members of the point group.
    void addToPointGroup(const std::string& name, const std::vector<Offset>& offsets) {
        addToGroup(attr::AttributeOwner::POINT, name, offsets);
    }
    /// @brief Marks the given offsets as members of the primitive group.
    void addToPrimitiveGroup(const std::string& name, const std::vector<Offset>& offsets) {
        addToGroup(attr::AttributeOwner::PRIMITIVE, name, offsets);
    }

    String getPath() const { return path_; };
    void setPath(const String &path) { path_ = path; };

  protected:
    virtual attr::attribVector &getAttributeStore(const attr::AttributeOwner &owner);
    virtual const attr::attribVector &getAttributeStore(const attr::AttributeOwner &owner) const;
    virtual attr::attribVector &getGroupStore(const attr::AttributeOwner &owner);
    virtual const attr::attribVector &getGroupStore(const attr::AttributeOwner &owner) const;
    attr::attribVector deepCopyAttributes(attr::attribVector source);

    /**
     * @brief Returns the number of elements in the given owner's store.
     * @return The element count.
     */
    size_t getElementCount(const attr::AttributeOwner &owner) const;

    std::string path_ = "/prim";
    attr::attribVector pointAttributes_;
    attr::attribVector primitiveAttributes_;
    attr::attribVector pointGroups_;
    attr::attribVector primitiveGroups_;
};

using PrimPtr = std::shared_ptr<Primitive>;
} // namespace enzo::geo
