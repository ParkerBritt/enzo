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

template <typename T>
class AttributeHandle
{
public:
    ga::AttributeType type_;

    AttributeHandle(std::shared_ptr<Attribute> attribute)
    {
        type_ = attribute->getType();
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
        else
        {
                throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in AttributeHandle constructor");
        }
    }

    void addValue(T value)
    {
        // TODO:make this private (primitive friend classes only)
        data_->push_back(value);
    }


    void reserve(std::size_t newCap)
    {
        data_->reserve(newCap);
    }

    // TODO: replace with iterator
    std::vector<T> getAllValues() const
    {
        return {data_->begin(), data_->end()};
    }

    size_t getSize() const
    {
        return data_->size();
    }

    T getValue(size_t pos) const
    {
        // TODO:protect against invalid positions
        // TODO: cast types
        return (*data_)[pos];
    }

    void setValue(size_t pos, const T& value)
    {
        // TODO:protect against invalid positions
        // TODO: cast types
        (*data_)[pos] = value;
    }
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

    std::string name_="";

    std::shared_ptr<StoreContainer<T>> data_;

    // int typeID_;

};

using AttributeHandleInt = AttributeHandle<bt::intT>;
using AttributeHandleFloat = AttributeHandle<bt::floatT>;
using AttributeHandleVector3 = AttributeHandle<enzo::bt::Vector3>;
using AttributeHandleBool = AttributeHandle<enzo::bt::boolT>;

template <typename T>
class AttributeHandleRO
{
public:
    ga::AttributeType type_;

    AttributeHandleRO(std::shared_ptr<const Attribute> attribute)
    {
        type_ = attribute->getType();
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
        else
        {
                throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in AttributeHandle constructor");
        }

    }

    // TODO: replace with iterator
    std::vector<T> getAllValues() const
    {
        return {data_->begin(), data_->end()};
    }

    size_t getSize() const
    {
        return data_->size();
    }

    T getValue(size_t pos) const
    {
        // TODO:protect against invalid positions
        // TODO: cast types
        return (*data_)[pos];
    }

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

    std::string name_="";

    std::shared_ptr<StoreContainer<T>> data_;

    // int typeID_;

};

using AttributeHandleInt = AttributeHandle<bt::intT>;
using AttributeHandleFloat = AttributeHandle<bt::floatT>;
using AttributeHandleVector3 = AttributeHandle<enzo::bt::Vector3>;
using AttributeHandleBool = AttributeHandle<enzo::bt::boolT>;


}
