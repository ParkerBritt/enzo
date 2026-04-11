#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <optional>
#include <vector>
#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include "tbb/concurrent_vector.h"

#include <iostream>


namespace enzo::ga{

/**
* @class enzo::ga::AttributeHandle
* @brief Read write accessor for #enzo::ga::Attribute
*
* @tparam T C++ value type matching the Attribute’s logical type
*           (e.g., bt::intT, bt::floatT, bt::Vector3, bt::boolT).
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
template <typename T>
class AttributeHandle
{
public:
    ga::AttributeType type_;

    /**
    * @brief Construct a new typed handle linked to a target attribute
    *
    * @param attribute The target attribute this handle will modify
    *
    */
    AttributeHandle(std::shared_ptr<Attribute> attribute)
    {
        if(attribute == nullptr) throw std::runtime_error("Cannot pass empty pointer to AttributeHandle constructor");
        type_ = attribute->getType();
        name_ = attribute->getName();
        // get attribute data pointer
        // TODO: check types match
        // TODO: add the other types

        // int
        if constexpr (std::is_same<bt::intT, T>::value)
        {
            data_=attribute->intStore_;
        }

        // float
        else if constexpr (std::is_same<bt::floatT, T>::value)
        {
            data_=attribute->floatStore_;
        }

        // vector 3
        else if constexpr (std::is_same<enzo::bt::Vector3, T>::value)
        {
            data_=attribute->vector3Store_;
        }
        else if constexpr (std::is_same<enzo::bt::boolT, T>::value)
        {
            data_=attribute->boolStore_;
        }
        else if constexpr (std::is_same<enzo::bt::Matrix4, T>::value)
        {
            data_=attribute->matrix4Store_;
        }
        else
        {
                throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in AttributeHandle constructor");
        }
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
    // * This is important when adding many elements to the attribute as automatic resizing is expensive.
    // *
    // * @param newCap The new maximum number of elements the attribute can hold before needing to automatically allocate more.
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
    * If the current size is greater than count, the container is reduced to its first count elements.
    * 
    * If the current size is less than count, then:
    * 
    * 1) Additional default-inserted elements are appended.
    * 2) Additional copies of value are appended.
    * 
    * @important This is important when adding many elements to the attribute as automatic resizing is expensive.
    *
    * @param newCap The new maximum number of elements the attribute can hold before needing to automatically allocate more.
    *
    */
    void resize(std::size_t newSize)
    {
        data_->resize(newSize);
    }


    // TODO: replace with iterator
    /**
    * @brief Gets a vector containing all the values stored in this attribute.
    *
    * @todo Replace with an iterator for accessing many values.
    *
    * @returns A vector containing all the values stored in this attribute.
    *
    */
    std::vector<T> getAllValues() const
    {
        return {data_->begin(), data_->end()};
    }

    /**
    * @brief Gets the number of element stored in the attribute
    */
    size_t getSize() const
    {
        return data_->size();
    }

    /**
    * @brief Gets the value at a given offset
    * @returns The value stored at a given offset
    * @todo protect against invalid positions
    * @todo Add implicit casting between types (eg. if T is int but the parameter's #ga::AttributeType is floatT 5.3 return 5)
    */
    T getValue(size_t offset) const
    {
        if(offset >= data_->size()) throw std::out_of_range("Cannot get offset: " + std::to_string(offset) + " from size: " + std::to_string(data_->size()) + " for attribute: " + name_);
        return (*data_)[offset];
    }

    /**
    * @brief Sets the value at a given offset.
    * @todo protect against invalid positions
    * @todo Add implicit casting between types (eg. if T is int but the parameter's #ga::AttributeType is floatT 5.3 return 5)
    */
    void setValue(size_t offset, const T& value)
    {
        (*data_)[offset] = value;
    }

    /**
    * @brief Returs the attribute name as a string
    */
    std::string getName() const
    {
        return name_;
    }



private:
    // private attributes are attributes that are hidden from the user
    // for internal use
    bool private_=false;
    // hidden attributes are user accessible attributes that the user may
    // or may want to use
    bool hidden_=false;
    // allows the user to read the attributeHandle but not modify it
    bool readOnly_=false;

    std::string name_;

    std::shared_ptr<StoreContainer<T>> data_;

    // int typeID_;

};

using AttributeHandleInt = AttributeHandle<bt::intT>;
using AttributeHandleFloat = AttributeHandle<bt::floatT>;
using AttributeHandleVector3 = AttributeHandle<enzo::bt::Vector3>;
using AttributeHandleBool = AttributeHandle<enzo::bt::boolT>;
using AttributeHandleMatrix4 = AttributeHandle<enzo::bt::Matrix4>;

template <typename T>
/**
* @brief Read only accessor for #enzo::ga::Attribute
* @copydetails AttributeHandle
*/
class AttributeHandleRO
{
public:
    ga::AttributeType type_;

    /// @copydoc AttributeHandle::AttributeHandle
    AttributeHandleRO(std::shared_ptr<const Attribute> attribute)
    {
        type_ = attribute->getType();
        name_ = attribute->getName();
        // get attribute data pointer
        // TODO: check types match
        // TODO: add the other types

        // int
        if constexpr (std::is_same<bt::intT, T>::value)
        {
            data_=attribute->intStore_;
        }

        // float
        else if constexpr (std::is_same<bt::floatT, T>::value)
        {
            data_=attribute->floatStore_;
        }

        // vector 3
        else if constexpr (std::is_same<enzo::bt::Vector3, T>::value)
        {
            data_=attribute->vector3Store_;
        }
        else if constexpr (std::is_same<enzo::bt::boolT, T>::value)
        {
            data_=attribute->boolStore_;
        }
        else if constexpr (std::is_same<enzo::bt::Matrix4, T>::value)
        {
            data_=attribute->matrix4Store_;
        }
        else
        {
                throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in AttributeHandle constructor");
        }

    }

    /// @copydoc AttributeHandle::getAllValues
    std::vector<T> getAllValues() const
    {
        return {data_->begin(), data_->end()};
    }

    /// @copydoc AttributeHandle::getSize
    size_t getSize() const
    {
        return data_->size();
    }

    /// @copydoc AttributeHandle::getValue
    T getValue(size_t offset) const
    {
        // TODO:protect against invalid positions
        // TODO: cast types
        // TODO: consider removing range check for faster reads
        if(offset >= data_->size()) throw std::out_of_range("Cannot get offset: " + std::to_string(offset) + " from size: " + std::to_string(data_->size()) + " for attribute: " + name_);
        return (*data_)[offset];
    }

    /// @copydoc AttributeHandle::getName
    std::string getName() const
    {
        return name_;
    }



private:
    // private attributes are attributes that are hidden from the user
    // for internal use
    bool private_=false;
    // hidden attributes are user accessible attributes that the user may
    // or may want to use
    bool hidden_=false;
    // allows the user to read the attributeHandle but not modify it
    bool readOnly_=false;

    std::string name_;

    std::shared_ptr<StoreContainer<T>> data_;

    // int typeID_;

};

using AttributeHandleInt = AttributeHandle<bt::intT>;
using AttributeHandleFloat = AttributeHandle<bt::floatT>;
using AttributeHandleVector3 = AttributeHandle<enzo::bt::Vector3>;
using AttributeHandleBool = AttributeHandle<enzo::bt::boolT>;
using AttributeHandleMatrix4 = AttributeHandle<enzo::bt::Matrix4>;


}
