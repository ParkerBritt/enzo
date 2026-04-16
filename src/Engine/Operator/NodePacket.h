#pragma once
#include "Engine/Operator/Primitive.h"
#include <memory>
#include <vector>

namespace enzo {

class NodePacket {
  public:
    // Lazy forward iterator that flattens transforms across all primitives.
    // See NodePacket.cpp for Iterator method implementations.
    struct Transforms {
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Transform;

            Iterator(std::vector<std::shared_ptr<geo::Primitive>> &primitives,
                     TransformClass transformClass, size_t primIdx);

            Transform operator*() const;
            Iterator &operator++();
            Iterator operator++(int);
            friend bool operator==(const Iterator &a, const Iterator &b) {
                return a.primIdx_ == b.primIdx_ && a.offset_ == b.offset_;
            }
            friend bool operator!=(const Iterator &a, const Iterator &b) { return !(a == b); }

          private:
            std::vector<std::shared_ptr<geo::Primitive>> &primitives_;
            TransformClass transformClass_;
            size_t primIdx_;
            ga::Offset offset_ = 0;
            std::shared_ptr<ga::Attribute> curAttrib_;
            size_t curSize_ = 0;

            void advance();
        };

        Transforms(std::vector<std::shared_ptr<geo::Primitive>> &primitives,
                   TransformClass transformClass)
            : primitives_(primitives), transformClass_(transformClass) {}

        Iterator begin() { return Iterator(primitives_, transformClass_, 0); }
        Iterator end() { return Iterator(primitives_, transformClass_, primitives_.size()); }

      private:
        std::vector<std::shared_ptr<geo::Primitive>> &primitives_;
        TransformClass transformClass_;
    };

    void addPrimitive(std::shared_ptr<enzo::geo::Primitive> primitive);
    void attemptMerge(std::shared_ptr<geo::Primitive> prim);

    std::shared_ptr<enzo::geo::Primitive> getPrimitive(unsigned int index);
    std::shared_ptr<const enzo::geo::Primitive> getPrimitive(unsigned int index) const;
    std::shared_ptr<geo::Primitive> getPrimAtPath(const std::string &path);
    std::shared_ptr<const geo::Primitive> getPrimAtPath(const std::string &path) const;

    const std::vector<std::shared_ptr<enzo::geo::Primitive>> &getPrimitives() const {
        return primitives_;
    }
    std::vector<std::shared_ptr<enzo::geo::Primitive>> getPrimitives(enzo::geo::PrimType type) const;

    Transforms getTransforms(TransformClass transformClass) {
        return Transforms(primitives_, transformClass);
    }

    size_t size() const;
    NodePacket deepCopy() const;

  private:
    std::vector<std::shared_ptr<enzo::geo::Primitive>> primitives_;
};

} // namespace enzo
