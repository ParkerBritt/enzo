#include "Engine/Operator/Attribute.h"
#include "Engine/Types.h"
#include <memory>
#include <stdexcept>
#include <optional>

namespace enzo {



attr::Attribute::Attribute(std::string name, attr::AttributeType type, bool intrinsic, bool isPrivate)
: name_{name}, type_{type}, intrinsic_{intrinsic}, private_{isPrivate}
{
    switch(type_)
    {
        case(AttrType::intT):
            store_ = std::make_shared<StoreContainer<intT>>();
            typeSize_ = 1;
            break;
        case(AttrType::floatT):
            store_ = std::make_shared<StoreContainer<floatT>>();
            typeSize_ = 1;
            break;
        case(AttrType::vectorT):
            store_ = std::make_shared<StoreContainer<Vector3>>();
            typeSize_ = 3;
            break;
        case(AttrType::boolT):
            store_ = std::make_shared<StoreContainer<boolT>>();
            typeSize_ = 1;
            break;
        case(AttrType::matrixT):
            store_ = std::make_shared<StoreContainer<Matrix4>>();
            typeSize_ = 16;
            break;
        default:
            throw std::runtime_error("Type " + std::to_string(static_cast<int>(type_)) + " was not properly accounted for in Attribute constructor");
    }
}

unsigned int attr::Attribute::Attribute::getTypeSize() const
{
    return typeSize_;
}


void attr::Attribute::resize(size_t size)
{
    std::visit([&](auto& storePtr) { storePtr->resize(size); }, store_);
}

void attr::Attribute::compact(const std::vector<bool>& keep)
{
    std::visit([&](auto& storePtr) {
        auto& store = *storePtr;
        const size_t n = std::min(store.size(), keep.size());

        // Walk the store and pack kept entries to the left.
        size_t writeIdx = 0;
        for (size_t i = 0; i < n; ++i)
        {
            if (!keep[i]) continue;
            if (writeIdx != i) store[writeIdx] = store[i];
            ++writeIdx;
        }

        // Drop the trailing entries that were left behind.
        store.resize(writeIdx);
    }, store_);
}

bool attr::Attribute::isIntrinsic() const
{
    return intrinsic_;
}

bool attr::Attribute::isPrivate() const
{
    return private_;
}



attr::Attribute::Attribute(const Attribute& other)
{
    type_ = other.type_;
    private_ = other.private_;
    // hidden_ = other.hidden_;
    // readOnly_ = other.readOnly_;
    intrinsic_ = other.intrinsic_;
    name_ = other.name_;
    typeSize_ = other.typeSize_;

    std::visit([&](const auto& storePtr) {
        using StorePtr = std::decay_t<decltype(storePtr)>;
        using Vec = typename StorePtr::element_type;
        store_ = std::make_shared<Vec>(*storePtr);
    }, other.store_);
}


Vector3 attr::Attribute::getVector3(Offset offset) const
{
    return (*std::get<std::shared_ptr<StoreContainer<Vector3>>>(store_))[offset];
}

Matrix4 attr::Attribute::getMatrix4(Offset offset) const
{
    return (*std::get<std::shared_ptr<StoreContainer<Matrix4>>>(store_))[offset];
}

size_t attr::Attribute::getSize() const
{
    return std::visit([](const auto& storePtr) -> size_t { return storePtr->size(); }, store_);
}

attr::AttributeType attr::Attribute::getType() const
{
    return type_;
}

std::string attr::Attribute::getName() const
{
    return name_;
}

} // namespace enzo
