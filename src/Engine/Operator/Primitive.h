#pragma once
#include "Engine/Operator/Attribute.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Operator/Point.h"
#include "Engine/Operator/Transform.h"
#include "Engine/Types.h"
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
            using value_type = ga::Offset;

            value_type operator*() const { return curOffset_; }

            Iterator(Primitive &primitive, ga::Offset current)
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
            ga::Offset curOffset_ = 0;
            void skipInvalid() {
                const ga::Offset end = primitive_.getNumPoints();
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
    virtual void applyTransform(const bt::Matrix4 &mat, TransformClass transformClass) = 0;
    void applyTransform(const Transform &transform, TransformClass transformClass) {
        applyTransform(transform.getMatrix(), transformClass);
    }
    virtual bool canMerge() const { return false; }
    virtual void merge(std::shared_ptr<Primitive> other) {}
    void incrementVersion();

    virtual bool hasPoints() const { return false; }
    ga::Offset getNumPoints() const { return getElementCount(ga::AttributeOwner::POINT); }
    virtual PointOffsets getPoints() { return PointOffsets(*this); }
    virtual bool isValidPoint(ga::Offset offset) const { return true; }
    virtual void deletePoints(const std::vector<ga::Offset>& pointOffsets) {}

    /**
     * @brief Compacts storage, removing entries marked invalid so offsets
     * are contiguous again.
     */
    virtual void defragment() {}

    ga::AttributeHandle<bt::intT> addIntAttribute(ga::AttributeOwner owner, std::string name,
                                                  bool intrinsic = false);
    ga::AttributeHandleBool addBoolAttribute(ga::AttributeOwner owner, std::string name,
                                             bool intrinsic = false, bool isPrivate = false);
    ga::AttributeHandle<bt::Vector3> addVector3Attribute(ga::AttributeOwner owner, std::string name,
                                                         bool intrinsic = false);
    ga::AttributeHandle<bt::Matrix4> addMatrix4Attribute(ga::AttributeOwner owner, std::string name,
                                                         bool intrinsic = false);

    std::shared_ptr<ga::Attribute> getAttribByName(ga::AttributeOwner owner, std::string name,
                                                   bool includeIntrinsics = false);
    const size_t getNumAttributes(const ga::AttributeOwner owner) const;
    std::weak_ptr<const ga::Attribute> getAttributeByIndex(ga::AttributeOwner owner,
                                                           unsigned int index) const;
    bool attributeExists(ga::AttributeOwner owner, std::string name);

    /**
     * @brief Creates a group on the given owner.
     *
     * Groups are boolean flags that mark elements as members. A group
     * and a regular attribute can share a name without colliding.
     *
     * @return Handle to the new group.
     */
    ga::AttributeHandleBool createGroup(ga::AttributeOwner owner, std::string name);
    /**
     * @brief Marks the given offsets as members of the group.
     */
    void addToGroup(ga::AttributeOwner owner, const std::string& name,
                    const std::vector<ga::Offset>& offsets);
    /**
     * @brief Looks up a group by name.
     * @return The matching group, or nullptr if no such group exists.
     */
    std::shared_ptr<ga::Attribute> getGroupByName(ga::AttributeOwner owner,
                                                  const std::string& name);

    /// @brief Creates a point group.
    /// @return Handle to the new group.
    ga::AttributeHandleBool createPointGroup(std::string name) {
        return createGroup(ga::AttributeOwner::POINT, std::move(name));
    }
    /// @brief Creates a primitive group.
    /// @return Handle to the new group.
    ga::AttributeHandleBool createPrimitiveGroup(std::string name) {
        return createGroup(ga::AttributeOwner::PRIMITIVE, std::move(name));
    }
    /// @brief Marks the given offsets as members of the point group.
    void addToPointGroup(const std::string& name, const std::vector<ga::Offset>& offsets) {
        addToGroup(ga::AttributeOwner::POINT, name, offsets);
    }
    /// @brief Marks the given offsets as members of the primitive group.
    void addToPrimitiveGroup(const std::string& name, const std::vector<ga::Offset>& offsets) {
        addToGroup(ga::AttributeOwner::PRIMITIVE, name, offsets);
    }

    bt::String getPath() const { return path_; };
    void setPath(const bt::String &path) { path_ = path; };

  protected:
    virtual ga::attribVector &getAttributeStore(const ga::AttributeOwner &owner);
    virtual const ga::attribVector &getAttributeStore(const ga::AttributeOwner &owner) const;
    virtual ga::attribVector &getGroupStore(const ga::AttributeOwner &owner);
    virtual const ga::attribVector &getGroupStore(const ga::AttributeOwner &owner) const;
    ga::attribVector deepCopyAttributes(ga::attribVector source);

    /**
     * @brief Returns the number of elements in the given owner's store.
     * @return The element count.
     */
    size_t getElementCount(const ga::AttributeOwner &owner) const;

    std::string path_ = "/prim";
    ga::attribVector pointAttributes_;
    ga::attribVector primitiveAttributes_;
    ga::attribVector pointGroups_;
    ga::attribVector primitiveGroups_;
};

using PrimPtr = std::shared_ptr<Primitive>;
} // namespace enzo::geo
