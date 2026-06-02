#pragma once
#include "Engine/Selection/IndexSet.h"
#include "Engine/Primitives/Primitive.h"
#include "Engine/Core/Types.h"
#include <memory>
#include <string>

namespace enzo {

/**
 * @brief Abstract base for one piece of a Selection expression.
 *
 * A Selection is a comma separated list of components. Each component knows
 * how to answer membership queries for a primitive and its faces, points,
 * and vertices. Path based components match a primitive by path and use
 * explicit index sets. Group based components look up named groups on the
 * primitive.
 */
class SelectionComponent {
  public:
    /**
     * @brief Parses one component string and returns the appropriate subclass.
     */
    static std::unique_ptr<SelectionComponent> fromString(std::string_view string);
    /**
     * @brief Creates a group based component for the named group.
     */
    static std::unique_ptr<SelectionComponent> fromGroup(std::string_view name);

    virtual ~SelectionComponent() = default;

    virtual bool containsPrim(const geo::Primitive &prim) const = 0;
    // When `inverted` is true the answer is flipped, but only for the
    // element type the component actually talks about.
    //
    // `index` is the compacted element number (counting only valid elements)
    // and `offset` is the raw storage offset. Path components match against the
    // compacted index, while group components read group bools keyed by offset.
    virtual bool containsFace(const geo::Primitive &prim, Index index, Offset offset,
                              bool inverted = false) const = 0;
    virtual bool containsPoint(const geo::Primitive &prim, Index index, Offset offset,
                               bool inverted = false) const = 0;
    virtual bool containsVertex(const geo::Primitive &prim, Index index, Offset offset,
                                bool inverted = false) const = 0;
    /**
     * @brief Whether the component selects the prim as a whole.
     *
     * Whole prim selection means every face, point, and vertex of a matched
     * prim is implicitly included. Group components consult the primitive
     * group flag for this answer, which is why the prim is required.
     */
    virtual bool isWholePrim(const geo::Primitive &prim) const = 0;

  protected:
    SelectionComponent() = default;
};

/**
 * @brief Selection component anchored to a primitive path plus optional
 *        per element index sets.
 */
class PathSelectionComponent : public SelectionComponent {
  public:
    static std::unique_ptr<PathSelectionComponent> parse(std::string_view string);

    bool containsPrim(const geo::Primitive &prim) const override;
    bool containsFace(const geo::Primitive &prim, Index index, Offset offset,
                      bool inverted = false) const override;
    bool containsPoint(const geo::Primitive &prim, Index index, Offset offset,
                       bool inverted = false) const override;
    bool containsVertex(const geo::Primitive &prim, Index index, Offset offset,
                        bool inverted = false) const override;
    bool isWholePrim(const geo::Primitive &prim) const override;

  private:
    PathSelectionComponent() = default;
    std::string primPath_;
    std::shared_ptr<IndexSet> points_;
    std::shared_ptr<IndexSet> faces_;
    std::shared_ptr<IndexSet> vertices_;
};

/**
 * @brief Selection component that resolves membership through a named group.
 */
class GroupSelectionComponent : public SelectionComponent {
  public:
    static std::unique_ptr<GroupSelectionComponent> create(std::string_view name);

    bool containsPrim(const geo::Primitive &prim) const override;
    bool containsFace(const geo::Primitive &prim, Index index, Offset offset,
                      bool inverted = false) const override;
    bool containsPoint(const geo::Primitive &prim, Index index, Offset offset,
                       bool inverted = false) const override;
    bool containsVertex(const geo::Primitive &prim, Index index, Offset offset,
                        bool inverted = false) const override;
    bool isWholePrim(const geo::Primitive &prim) const override;

  private:
    GroupSelectionComponent() = default;
    std::string groupName_;
};

} // namespace enzo
