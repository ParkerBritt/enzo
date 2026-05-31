#pragma once
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <any>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace enzo::prm {

class Template {
  public:
    Template(enzo::prm::Type type, prm::Name name);
    Template(enzo::prm::Type type, prm::Name name, unsigned int vectorSize);
    Template(enzo::prm::Type type, prm::Name name, std::vector<prm::Default> defaults,
             unsigned int vectorSize = 1,
             std::vector<prm::Range> ranges = std::vector<prm::Range>());
    Template(enzo::prm::Type type, prm::Name name, prm::Default theDefault,
             unsigned int vectorSize = 1, Range range = Range());
    // get name and get token are identical
    enzo::bt::String getName() const;
    enzo::bt::String getToken() const;
    enzo::bt::String getLabel() const;
    const prm::Default getDefault(unsigned int index = 0) const;
    const prm::Range &getRange(unsigned int index = 0) const;
    const prm::Type getType() const;
    const unsigned int getSize() const;
    const unsigned int getNumDefaults() const;

    Direction getDirection() const;
    const std::vector<Template> &getChildren() const;
    const bool isContainer() const;

    enzo::bt::String getTooltip() const;
    enzo::bt::String getDocumentation() const;
    bool isLabelHidden() const;
    bool isBackgroundEnabled() const;

    // Chainable setters. Mutate in place and return *this so a Template can
    // be configured inline at construction.
    Template &setTooltip(bt::String tooltip);
    Template &setDocumentation(bt::String documentation);
    Template &setDirection(Direction direction);
    Template &addParm(Template child);
    Template &setLabelHidden(bool hidden);
    Template &setBackgroundEnabled(bool enabled);

    // Style structs may hold parameters with change signals, which cannot
    // be copied. Wrapping in a shared pointer lets the style live inside
    // the std::any without ever being copied.
    template <typename T> Template &setStyle(T style) {
        style_ = std::make_shared<T>(std::move(style));
        return *this;
    }

    const std::any &getStyle() const;

  private:
    enzo::prm::Type type_;
    std::vector<prm::Default> defaults_;
    std::vector<prm::Range> ranges_;
    prm::Name name_;
    unsigned int vectorSize_;

    bt::String tooltip_;
    bt::String documentation_;

    bool labelHidden_ = false;
    bool backgroundEnabled_ = true;

    Direction direction_ = Direction::HORIZONTAL;
    std::vector<Template> children_;
    std::any style_;
    inline const static std::unordered_set<prm::Type> containerTypes_ = {prm::Type::GROUP};
    inline const static std::unordered_set<prm::Type> backgroundDisabledByDefault_ = {
        prm::Type::GROUP, prm::Type::XYZ};
};

} // namespace enzo::prm
