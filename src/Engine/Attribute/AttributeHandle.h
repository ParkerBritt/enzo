#pragma once
#include "Engine/Attribute/Attribute.h"
#include "Engine/Core/Types.h"
#include "tbb/concurrent_vector.h"
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <iostream>

namespace enzo::attr {

/**
 * @class enzo::attr::AttributeHandle
 * @brief Read write accessor for #enzo::attr::Attribute
 *
 * @tparam T C++ value type matching the Attribute’s logical type
 *           (e.g., intT, floatT, Vector3, boolT).
 *
 * An Attribute Handle is a typed view into an attribute’s storage.
 * It binds at construction to a concrete type and exposes operations
 * like reserving capacity, appending values, and reading/writing by
 * index. Because the handle uses templating, most misuse is caught
 * at compile time, and runtime guards raise errors if an attribute/handle
 * type combination isn’t accounted for. In the future implicit casting
 * can be added for convenience. Handles don’t own data,
 * they just reference the attribute’s storage.
 *
 * There is also a read-only handle variant that provides the same
 * typed accessors without mutation. This is useful when an operator
 * needs to inspect data but must not modify it, when the engine exposes
 * attributes to user code with limited permissions, or when implementing
 * const member functions that require attribute access.
 *
 * Together, attributes define the schema and storage, while handles
 * provide the typed access that nodes and tools use to operate on data directly.
 */
template <typename T> class AttributeHandle
{
  public:
    attr::AttributeType type_;

    /**
     * @brief Construct a new typed handle linked to a target attribute
     *
     * @param attribute The target attribute this handle will modify
     *
     */
    AttributeHandle(std::shared_ptr<Attribute> attribute)
    {
        if (attribute == nullptr)
            throw std::runtime_error("Cannot pass empty pointer to AttributeHandle constructor");
        type_ = attribute->getType();
        name_ = attribute->getName();
        data_ = std::get<std::shared_ptr<StoreContainer<T>>>(attribute->store_);
    }

    /**
     * @brief Adds an element to the end of the attribute.
     *
     * @param The element value the value to add to the attribute.
     *
     */
    void addValue(T value)
    {
        // TODO:make this private (primitive friend classes only)
        data_->push_back(value);
    }

    // /**
    // * @brief Reserves more space in the attribute to add new elements
    // *
    // * This is important when adding many elements to the attribute as automatic resizing is
    // expensive.
    // *
    // * @param newCap The new maximum number of elements the attribute can hold before needing to
    // automatically allocate more.
    // *
    // */
    // void reserve(std::size_t newCap)
    // {
    //     data_->reserve(newCap);
    // }

    /**
     * @brief Resizes more space in the attribute to add new elements
     *
     * Resizes the container to contain count elements, does nothing if count == size().
     *
     * If the current size is greater than count, the container is reduced to its first count
     * elements.
     *
     * If the current size is less than count, then:
     *
     * 1) Additional default-inserted elements are appended.
     * 2) Additional copies of value are appended.
     *
     * @important This is important when adding many elements to the attribute as automatic resizing
     * is expensive.
     *
     * @param newCap The new maximum number of elements the attribute can hold before needing to
     * automatically allocate more.
     *
     */
    void resize(std::size_t newSize) { data_->resize(newSize); }

    // TODO: replace with iterator
    /**
     * @brief Gets a vector containing all the values stored in this attribute.
     *
     * @todo Replace with an iterator for accessing many values.
     *
     * @returns A vector containing all the values stored in this attribute.
     *
     */
    std::vector<T> getAllValues() const { return {data_->begin(), data_->end()}; }

    /**
     * @brief Gets the number of element stored in the attribute
     */
    size_t getSize() const { return data_->size(); }

    /**
     * @brief Gets the value at a given offset.
     * @return The value stored at the offset.
     * @todo protect against invalid positions
     * @todo Add implicit casting between types (eg. if T is int but the parameter's
     * #attr::AttributeType is floatT 5.3 return 5)
     */
    T getValue(size_t offset) const { return (*data_)[offset]; }

    /**
     * @brief Zero copy element access. Prefer in hot loops over getValue.
     * @return Reference to the value at the offset. Invalid after a mutation that grows storage.
     *
     * Not available for bool handles since std::vector<bool> is bit packed and has no real
     * reference.
     */
    const T& operator[](size_t offset) const
        requires(!std::is_same_v<T, boolT>)
    {
        return (*data_)[offset];
    }

    /**
     * @brief Contiguous read only view over all stored values.
     * @return Span over the storage. Invalid after a mutation that grows storage.
     */
    std::span<const T> getSpan() const { return {data_->data(), data_->size()}; }

    /**
     * @brief Sets the value at a given offset.
     * @todo protect against invalid positions
     * @todo Add implicit casting between types (eg. if T is int but the parameter's
     * #attr::AttributeType is floatT 5.3 return 5)
     */
    void setValue(size_t offset, const T& value) { (*data_)[offset] = value; }

    /**
     * @brief Returs the attribute name as a string
     */
    std::string getName() const { return name_; }

  private:
    // private attributes are attributes that are hidden from the user
    // for internal use
    bool private_ = false;
    // hidden attributes are user accessible attributes that the user may
    // or may want to use
    bool hidden_ = false;
    // allows the user to read the attributeHandle but not modify it
    bool readOnly_ = false;

    std::string name_;

    std::shared_ptr<StoreContainer<T>> data_;

    // int typeID_;
};

using AttributeHandleInt = AttributeHandle<intT>;
using AttributeHandleFloat = AttributeHandle<floatT>;
using AttributeHandleVector3 = AttributeHandle<enzo::Vector3>;
using AttributeHandleBool = AttributeHandle<enzo::boolT>;
using AttributeHandleMatrix4 = AttributeHandle<enzo::Matrix4>;

template <typename T>
/**
 * @brief Read only accessor for #enzo::attr::Attribute
 * @copydetails AttributeHandle
 */
class AttributeHandleRO
{
  public:
    attr::AttributeType type_;

    /// @copydoc AttributeHandle::AttributeHandle
    AttributeHandleRO(std::shared_ptr<const Attribute> attribute)
    {
        type_ = attribute->getType();
        name_ = attribute->getName();
        data_ = std::get<std::shared_ptr<StoreContainer<T>>>(attribute->store_);
    }

    /// @copydoc AttributeHandle::getAllValues
    std::vector<T> getAllValues() const { return {data_->begin(), data_->end()}; }

    /// @copydoc AttributeHandle::getSize
    size_t getSize() const { return data_->size(); }

    /// @copydoc AttributeHandle::getValue
    T getValue(size_t offset) const
    {
        // TODO:protect against invalid positions
        // TODO: cast types
        // TODO: consider removing range check for faster reads
        if (offset >= data_->size())
            throw std::out_of_range(
                "Cannot get offset: " + std::to_string(offset) +
                " from size: " + std::to_string(data_->size()) + " for attribute: " + name_
            );
        return (*data_)[offset];
    }

    /// @copydoc AttributeHandle::operator[]
    const T& operator[](size_t offset) const
        requires(!std::is_same_v<T, boolT>)
    {
        return (*data_)[offset];
    }

    /// @copydoc AttributeHandle::getName
    std::string getName() const { return name_; }

  private:
    // private attributes are attributes that are hidden from the user
    // for internal use
    bool private_ = false;
    // hidden attributes are user accessible attributes that the user may
    // or may want to use
    bool hidden_ = false;
    // allows the user to read the attributeHandle but not modify it
    bool readOnly_ = false;

    std::string name_;

    std::shared_ptr<StoreContainer<T>> data_;

    // int typeID_;
};

using AttributeHandleInt = AttributeHandle<intT>;
using AttributeHandleFloat = AttributeHandle<floatT>;
using AttributeHandleVector3 = AttributeHandle<enzo::Vector3>;
using AttributeHandleBool = AttributeHandle<enzo::boolT>;
using AttributeHandleMatrix4 = AttributeHandle<enzo::Matrix4>;

} // namespace enzo::attr
