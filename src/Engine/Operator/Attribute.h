#pragma once
#include <oneapi/tbb/concurrent_vector.h>
#include <string>
#include <optional>
#include <string_view>
#include <vector>
#include "Engine/Types.h"
#include <memory>


namespace enzo{
    namespace ga{
        template <typename T>
        using StoreContainer = std::vector<T>;

        template <typename T>
        class AttributeHandle;

        class Attribute
        {
        public:
            Attribute(std::string name, ga::AttributeType type);
            Attribute(const Attribute& other);
            AttributeType getType() const;
            std::string getName() const;
            unsigned int getTypeSize() const;



            template <typename T>
            friend class AttributeHandle;
            template <typename T>
            friend class AttributeHandleRO;

        private:
            // private attributes are attributes that are hidden from the user
            // for internal use
            bool private_=false;
            // hidden attributes are user accessible attributes that the user may
            // or may want to use
            bool hidden_=false;
            // allows the user to read the attribute but not modify it
            bool readOnly_=false;

            ga::AttributeType type_;
            unsigned int typeSize_=1;

            std::string name_;

            // void* data_;

            // data stores
            std::shared_ptr<StoreContainer<bt::intT>> intStore_;
            std::shared_ptr<StoreContainer<bt::floatT>> floatStore_;
            std::shared_ptr<StoreContainer<enzo::bt::Vector3>> vector3Store_;
            std::shared_ptr<StoreContainer<enzo::bt::boolT>> boolStore_;
        };


    }
}
