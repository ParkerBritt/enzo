#pragma once
#include "Engine/Primitives/Primitive.h"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace enzo {

class NodePacket
{
  public:
    // Lazy forward iterator that flattens transforms across all primitives.
    // See NodePacket.cpp for Iterator method implementations.
    struct Transforms
    {
        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Transform;

            Iterator(
                std::vector<geo::PrimPtr>& primitives,
                TransformClass transformClass,
                size_t primIdx
            );

            Transform operator*() const;
            Iterator& operator++();
            Iterator operator++(int);
            friend bool operator==(const Iterator& a, const Iterator& b)
            {
                return a.primIdx_ == b.primIdx_ && a.offset_ == b.offset_;
            }
            friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

          private:
            std::vector<geo::PrimPtr>& primitives_;
            TransformClass transformClass_;
            size_t primIdx_;
            Offset offset_ = 0;
            std::shared_ptr<attr::Attribute> curAttrib_;
            size_t curSize_ = 0;
            std::vector<Transform> orientations_;

            void advance();
        };

        Transforms(std::vector<geo::PrimPtr>& primitives, TransformClass transformClass)
            : primitives_(primitives), transformClass_(transformClass)
        {
        }

        Iterator begin() { return Iterator(primitives_, transformClass_, 0); }
        Iterator end() { return Iterator(primitives_, transformClass_, primitives_.size()); }

      private:
        std::vector<geo::PrimPtr>& primitives_;
        TransformClass transformClass_;
    };

    /// @brief Adds the primitive, numbering its path when another primitive holds it.
    ///
    /// Adding three primitives at `/light` gives `/light`, `/light_1` and `/light_2`.
    void addPrimitive(std::shared_ptr<enzo::geo::Primitive> primitive);
    /// @brief Merges the primitive into the one at its path, or adds it when either cannot merge.
    void attemptMerge(geo::PrimPtr prim);

    std::shared_ptr<enzo::geo::Primitive> getPrimitive(unsigned int index);
    std::shared_ptr<const enzo::geo::Primitive> getPrimitive(unsigned int index) const;
    geo::PrimPtr getPrimAtPath(const std::string& path);
    std::shared_ptr<const geo::Primitive> getPrimAtPath(const std::string& path) const;

    const std::vector<std::shared_ptr<enzo::geo::Primitive>>& getPrimitives() const
    {
        return primitives_;
    }
    std::vector<geo::PrimPtr> getPrimitives(enzo::geo::PrimType type) const;
    /// @brief Moves the primitive to the path, numbering it when another primitive holds it.
    void movePrim(const geo::PrimPtr& primitive, const std::string& toPath);
    /// @brief Moves the primitive at one path to another, numbering it when another primitive holds it.
    void movePrim(const std::string& fromPath, const std::string& toPath);
    void removePrim(std::string path);

    Transforms getTransforms(TransformClass transformClass)
    {
        return Transforms(primitives_, transformClass);
    }

    size_t size() const;
    NodePacket deepCopy() const;

  private:
    /// @brief Returns the path, numbered when another primitive holds it, and marks it as held.
    std::string claimPath(const std::string& path);

    std::vector<geo::PrimPtr> primitives_;
    /// @brief The path of each primitive, with the next number to try when another asks for it.
    std::unordered_map<std::string, std::size_t> nextNumberByPath_;
};

} // namespace enzo
