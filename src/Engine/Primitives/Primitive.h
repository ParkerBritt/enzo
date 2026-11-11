#pragma once
#include "Engine/Attribute/Attribute.h"
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Attribute/Point.h"
#include "Engine/Attribute/Transform.h"
#include "Engine/Core/Types.h"
#include <memory>
#include <optional>
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
 * types have points. Use hasOwner() to check before accessing them.
 *
 * Subclasses (e.g. Mesh) add their own attribute owners and geometry.
 */
class Primitive
{
  public:
    // Point Iterator
    struct PointOffsets
    {
        PointOffsets(Primitive& primitive) : primitive_(primitive) {}
        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Offset;

            value_type operator*() const { return curOffset_; }

            Iterator(Primitive& primitive, Offset current)
                : primitive_(primitive), curOffset_(current)
            {
                skipInvalid();
            }

            // Prefix increment
            Iterator& operator++()
            {
                ++curOffset_;
                skipInvalid();
                return *this;
            }

            // Postfix increment
            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator& a, const Iterator& b)
            {
                return a.curOffset_ == b.curOffset_;
            };
            friend bool operator!=(const Iterator& a, const Iterator& b)
            {
                return a.curOffset_ != b.curOffset_;
            };

          private:
            enzo::geo::Primitive& primitive_;
            Offset curOffset_ = 0;
            void skipInvalid()
            {
                const Offset end = primitive_.getNumPoints();
                while (curOffset_ < end && !primitive_.isValidPoint(curOffset_))
                    ++curOffset_;
            }
        };
        Iterator begin() { return Iterator(primitive_, 0); }
        Iterator end() { return Iterator(primitive_, primitive_.getNumPoints()); }

      private:
        enzo::geo::Primitive& primitive_;
    };
    Primitive(std::string_view path = "/prim");
    /// @brief Copies the primitive, sharing every attribute until one side writes to it.
    Primitive(const Primitive& other) = default;
    Primitive& operator=(const Primitive& rhs) = default;
    virtual ~Primitive() = default;

    virtual PrimType getType() const = 0;
    /// @brief Returns a copy that shares every attribute until one side writes to it.
    virtual std::shared_ptr<Primitive> clone() const = 0;
    virtual TransformClass transformType() const = 0;
    virtual void applyTransform(const Matrix4& mat, TransformClass transformClass) = 0;
    void applyTransform(const Transform& transform, TransformClass transformClass)
    {
        applyTransform(transform.getMatrix(), transformClass);
    }
    virtual bool canMerge() const { return false; }
    virtual void merge(std::shared_ptr<Primitive> other) {}

    /// @brief Returns whether this primitive type stores elements of the owner.
    virtual bool hasOwner(attr::AttributeOwner owner) const
    {
        return owner == attr::AttributeOwner::PRIMITIVE;
    }

    /// @brief Returns the number of elements in the given owner's store.
    size_t getElementCount(const attr::AttributeOwner& owner) const;

    Offset getNumPoints() const { return getElementCount(attr::AttributeOwner::POINT); }
    virtual PointOffsets getPoints() { return PointOffsets(*this); }
    virtual bool isValidPoint(Offset offset) const { return true; }
    virtual void deletePoints(const std::vector<Offset>& pointOffsets) {}

    /**
     * @brief Compacts storage, removing entries marked invalid so offsets
     * are contiguous again.
     */
    virtual void defragment() {}

    /**
     * @brief Returns the attribute of this name and type, adding one when the name
     * is free.
     *
     * @throws std::runtime_error When the name belongs to an internal attribute or to an
     *         intrinsic of another type.
     * @note An ordinary attribute of another type under the same name is replaced,
     *       dropping its values.
     */
    std::shared_ptr<const attr::Attribute> addAttribute(
        attr::AttributeOwner owner,
        std::string name,
        attr::AttributeType type,
        bool intrinsic = false,
        bool isInternal = false
    );

    /**
     * @brief Returns the attribute of this name and type, adding one when the name
     * is free.
     *
     * @return The attribute, or nullptr when the name belongs to an internal attribute
     *         or to an intrinsic of another type.
     * @note An ordinary attribute of another type under the same name is replaced,
     *       dropping its values.
     */
    std::shared_ptr<const attr::Attribute> tryAddAttribute(
        attr::AttributeOwner owner,
        std::string name,
        attr::AttributeType type,
        bool intrinsic = false,
        bool isInternal = false
    );

    /**
     * @brief Returns a handle to the attribute of this type stored under this name,
     * adding one when the name is free.
     *
     * @throws std::runtime_error When the name belongs to an internal attribute or to an
     *         intrinsic of another type.
     * @note An ordinary attribute of another type under the same name is replaced,
     *       dropping its values.
     */
    template <typename T>
    attr::AttributeHandle<T> addAttribute(
        attr::AttributeOwner owner,
        std::string name,
        bool intrinsic = false,
        bool isInternal = false
    )
    {
        const attr::AttributeType type = attr::getAttributeType<T>();
        addAttribute(owner, name, type, intrinsic, isInternal);
        return *getWriteHandle<T>(owner, name, true);
    }

    /**
     * @brief Returns a handle to the attribute of this type stored under this name,
     * adding one when the name is free.
     *
     * @return The handle, or nothing when the name belongs to an internal attribute
     *         or to an intrinsic of another type.
     * @note An ordinary attribute of another type under the same name is replaced,
     *       dropping its values.
     */
    template <typename T>
    std::optional<attr::AttributeHandle<T>> tryAddAttribute(
        attr::AttributeOwner owner,
        std::string name,
        bool intrinsic = false,
        bool isInternal = false
    )
    {
        const attr::AttributeType type = attr::getAttributeType<T>();
        if (!tryAddAttribute(owner, name, type, intrinsic, isInternal)) return std::nullopt;
        return getWriteHandle<T>(owner, name, true);
    }

    /**
     * @brief Returns a handle that writes to the attribute of this name.
     *
     * @return The handle, or nothing when the primitive holds no attribute of this name.
     * @note Copies the attribute first when another primitive shares it.
     * @note A handle writes to the attribute as it was when the handle was taken. Take the
     *       handle again after copying the primitive, and avoid two handles on one attribute.
     * @note Take write handles before splitting work across threads, since taking one can
     *       replace the attribute the other threads hold.
     */
    template <typename T>
    std::optional<attr::AttributeHandle<T>> getWriteHandle(
        attr::AttributeOwner owner,
        std::string_view name,
        bool includeIntrinsics = false
    )
    {
        std::optional<size_t> index = getAttributeIndex(owner, name, includeIntrinsics);
        if (!index) return std::nullopt;
        attr::attribVector& attributes = getAttributeStore(owner);
        return attr::AttributeHandle<T>(unshareAttribute(attributes, *index));
    }

    /// @brief Returns the attribute of this name, or nullptr when the primitive holds none.
    std::shared_ptr<const attr::Attribute> getAttribByName(
        attr::AttributeOwner owner,
        std::string_view name,
        bool includeIntrinsics = false
    ) const;
    const size_t getNumAttributes(const attr::AttributeOwner owner) const;
    std::weak_ptr<const attr::Attribute>
    getAttributeByIndex(attr::AttributeOwner owner, unsigned int index) const;
    /**
     * @brief Returns every attribute on the owner.
     *
     * @note Internal attributes are left out, like @ref getAttributeByIndex.
     */
    std::vector<std::shared_ptr<const attr::Attribute>>
    getAttributes(attr::AttributeOwner owner, bool includeIntrinsics = false) const;
    bool attributeExists(attr::AttributeOwner owner, std::string_view name) const;

    /**
     * @brief Returns the group of this name on the given owner, adding one when the
     * name is free.
     *
     * @note Groups are boolean flags that mark elements as members. A group and an
     *       attribute can share a name without colliding.
     */
    attr::AttributeHandleBool addGroup(attr::AttributeOwner owner, std::string name);
    /**
     * @brief Marks the given offsets as members of the group.
     */
    void addToGroup(
        attr::AttributeOwner owner,
        const std::string& name,
        const std::vector<Offset>& offsets
    );
    /**
     * @brief Looks up a group by name.
     * @return The matching group, or nullptr if no such group exists.
     */
    std::shared_ptr<const attr::Attribute>
    getGroupByName(attr::AttributeOwner owner, const std::string& name) const;
    /**
     * @brief Returns a handle that writes to the group of this name.
     *
     * @return The handle, or nothing when the owner holds no group of this name.
     * @note Follows the same rules as @ref getWriteHandle.
     */
    std::optional<attr::AttributeHandleBool>
    getGroupWriteHandle(attr::AttributeOwner owner, std::string_view name);
    /**
     * @brief Returns how many groups live on the given owner.
     */
    size_t getNumGroups(attr::AttributeOwner owner) const;
    /**
     * @brief Returns the group at the given index in the owner's group store.
     */
    std::weak_ptr<const attr::Attribute>
    getGroupByIndex(attr::AttributeOwner owner, unsigned int index) const;
    /// @brief Returns every group on the owner.
    std::vector<std::shared_ptr<const attr::Attribute>> getGroups(attr::AttributeOwner owner) const;

    /**
     * @brief Adds each attribute and group of the source to this primitive, keeping its name and
     * type.
     *
     * @note Names this primitive already holds are left alone.
     */
    void addAttributesFrom(const Primitive& source, attr::AttributeOwner owner);

    /// @brief Returns the point group of this name, adding one when the name is free.
    attr::AttributeHandleBool addPointGroup(std::string name)
    {
        return addGroup(attr::AttributeOwner::POINT, std::move(name));
    }
    /// @brief Returns the primitive group of this name, adding one when the name is free.
    attr::AttributeHandleBool addPrimitiveGroup(std::string name)
    {
        return addGroup(attr::AttributeOwner::PRIMITIVE, std::move(name));
    }
    /// @brief Marks the given offsets as members of the point group.
    void addToPointGroup(const std::string& name, const std::vector<Offset>& offsets)
    {
        addToGroup(attr::AttributeOwner::POINT, name, offsets);
    }
    /// @brief Marks the given offsets as members of the primitive group.
    void addToPrimitiveGroup(const std::string& name, const std::vector<Offset>& offsets)
    {
        addToGroup(attr::AttributeOwner::PRIMITIVE, name, offsets);
    }

    String getPath() const { return path_; };
    void setPath(const String& path) { path_ = path; };

  protected:
    virtual attr::attribVector& getAttributeStore(const attr::AttributeOwner& owner);
    virtual const attr::attribVector& getAttributeStore(const attr::AttributeOwner& owner) const;
    virtual attr::attribVector& getGroupStore(const attr::AttributeOwner& owner);
    virtual const attr::attribVector& getGroupStore(const attr::AttributeOwner& owner) const;
    /// @brief Returns the value of the intrinsic primitive attribute of this name.
    template <typename T> T getPrimitiveIntrinsic(std::string_view name) const
    {
        return getValues<T>(*getAttribByName(attr::AttributeOwner::PRIMITIVE, name, true))[0];
    }

    /// @brief Sets the value of the intrinsic primitive attribute of this name.
    template <typename T> void setPrimitiveIntrinsic(std::string_view name, const T& value)
    {
        const attr::AttributeOwner owner = attr::AttributeOwner::PRIMITIVE;
        attr::attribVector& attributes = getAttributeStore(owner);
        getWritableValues<T>(attributes, *getAttributeIndex(owner, name, true))[0] = value;
    }

    /// @brief Copies the attribute at this index when another primitive shares it, then returns it.
    static attr::Attribute& unshareAttribute(attr::attribVector& attributes, size_t index);

    /// @brief Returns every attribute and group of the owner, each ready to write to.
    std::vector<attr::Attribute*> getWritableAttributesAndGroups(attr::AttributeOwner owner);

    /// @brief Returns the stored values of the attribute.
    template <typename T>
    static const attr::StoreContainer<T>& getValues(const attr::Attribute& attribute)
    {
        return *std::get<std::shared_ptr<attr::StoreContainer<T>>>(attribute.store_);
    }

    /// @brief Returns the values of the attribute at this index, unsharing it first.
    template <typename T>
    static attr::StoreContainer<T>& getWritableValues(attr::attribVector& attributes, size_t index)
    {
        attr::Attribute& attribute = unshareAttribute(attributes, index);
        return *std::get<std::shared_ptr<attr::StoreContainer<T>>>(attribute.store_);
    }

    /// @brief Returns the index of the attribute of this name in the owner's attributes.
    std::optional<size_t>
    getAttributeIndex(attr::AttributeOwner owner, std::string_view name, bool includeIntrinsics)
        const;

    std::string path_ = "/prim";
    attr::attribVector pointAttributes_;
    attr::attribVector primitiveAttributes_;
    attr::attribVector pointGroups_;
    attr::attribVector primitiveGroups_;
};

using PrimPtr = std::shared_ptr<Primitive>;
} // namespace enzo::geo
