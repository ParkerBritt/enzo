#include "Engine/Parameter/Template.h"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/Range.h"
#include <utility>

namespace enzo {

namespace {
// The fixed instance template every ramp carries. Each control point holds a
// position, a value, and an interpolation mode.
std::vector<prm::Template> rampInstanceTemplate()
{
    return {
        prm::Template(prm::Type::FLOAT, prm::Name("position", "Position")),
        prm::Template(prm::Type::FLOAT, prm::Name("value", "Value"), prm::Default(1)),
        prm::Template(
            prm::Type::DROPDOWN,
            prm::Name("interp", "Interpolation"),
            prm::Default("linear")
        )
            .setOptions({
                prm::Name("constant", "Constant"),
                prm::Name("linear", "Linear"),
            }),
    };
}
} // namespace

prm::Template::Template(prm::Type type, prm::Name name) : Template(type, name, prm::Default())
{
    ranges_.resize(vectorSize_);
}

prm::Template::Template(prm::Type type, prm::Name name, unsigned int vectorSize)
    : Template(type, name, prm::Default(), vectorSize)
{
    ranges_.resize(vectorSize_);
}

prm::Template::Template(
    prm::Type type,
    prm::Name name,
    std::vector<prm::Default> defaults,
    unsigned int vectorSize,
    std::vector<prm::Range> ranges
)
    : type_{type}, defaults_{defaults}, name_{name}, vectorSize_(vectorSize)
{
    if (ranges.size() < vectorSize_)
    {
        if (ranges.size() >= 1)
        {
            ranges.resize(vectorSize_, ranges[0]);
        }
        else
        {
            ranges.resize(vectorSize_);
        }
    }
    ranges_ = ranges;
    backgroundEnabled_ = !backgroundDisabledByDefault_.contains(type_);
}

prm::Template::Template(
    prm::Type type,
    prm::Name name,
    prm::Default theDefault,
    unsigned int vectorSize,
    Range range
)
    : type_{type}, name_{name}, vectorSize_(vectorSize)
{
    defaults_.resize(vectorSize_, theDefault);
    ranges_.resize(vectorSize_, range);
    backgroundEnabled_ = !backgroundDisabledByDefault_.contains(type_);

    // A ramp carries a fixed instance template the caller never builds, and
    // defaults to a linear zero to one curve callers can override.
    if (type_ == prm::Type::RAMP)
    {
        children_ = rampInstanceTemplate();
        instanceDefaults_["position"] = {prm::Default(0), prm::Default(1)};
        instanceDefaults_["value"] = {prm::Default(0), prm::Default(1)};
    }
}

const prm::Type prm::Template::getType() const { return type_; }

const unsigned int prm::Template::getSize() const { return vectorSize_; }

const prm::Default prm::Template::getDefault(unsigned int index) const
{
    return defaults_.at(index);
}

const prm::Range& prm::Template::getRange(unsigned int index) const { return ranges_.at(index); }

const std::vector<prm::Name>& prm::Template::getOptions() const { return options_; }

bool prm::Template::hasOptions() const { return !options_.empty(); }

const unsigned int prm::Template::getNumDefaults() const { return defaults_.size(); }

String prm::Template::getName() const { return name_.getToken(); }

String prm::Template::getToken() const { return name_.getToken(); }

String prm::Template::getLabel() const { return name_.getLabel(); }

prm::Direction prm::Template::getDirection() const { return direction_; }

const std::vector<prm::Template>& prm::Template::getChildren() const { return children_; }

String prm::Template::getTooltip() const { return tooltip_; }

String prm::Template::getDocumentation() const { return documentation_; }

bool prm::Template::isLabelHidden() const { return labelHidden_; }

prm::Template& prm::Template::setTooltip(String tooltip)
{
    tooltip_ = std::move(tooltip);
    return *this;
}

prm::Template& prm::Template::setDocumentation(String documentation)
{
    documentation_ = std::move(documentation);
    return *this;
}

prm::Template& prm::Template::setDirection(Direction direction)
{
    direction_ = direction;
    return *this;
}

prm::Template& prm::Template::setOptions(std::vector<prm::Name> options)
{
    options_ = std::move(options);

    // Sets default to first option if no default exists.
    bool hasStringDefault = !defaults_.empty() && !defaults_[0].getString().empty();
    if (!options_.empty() && !hasStringDefault)
    {
        for (prm::Default& theDefault : defaults_)
        {
            theDefault.setString(options_.front().getToken());
        }
    }
    return *this;
}

prm::Template& prm::Template::addParm(Template child)
{
    children_.push_back(std::move(child));
    return *this;
}

prm::Template& prm::Template::setInstanceDefault(std::string fieldToken, std::vector<Default> defaults)
{
    instanceDefaults_[std::move(fieldToken)] = std::move(defaults);
    return *this;
}

std::optional<prm::Default>
prm::Template::getInstanceDefault(const std::string& fieldToken, unsigned int instanceIndex) const
{
    auto entry = instanceDefaults_.find(fieldToken);
    if (entry == instanceDefaults_.end() || instanceIndex >= entry->second.size())
        return std::nullopt;
    return entry->second[instanceIndex];
}

prm::Template& prm::Template::setLabelHidden(bool hidden)
{
    labelHidden_ = hidden;
    return *this;
}

prm::Template& prm::Template::setBackgroundEnabled(bool enabled)
{
    backgroundEnabled_ = enabled;
    return *this;
}

bool prm::Template::isBackgroundEnabled() const { return backgroundEnabled_; }

const std::any& prm::Template::getStyle() const { return style_; }

const bool prm::Template::isContainer() const { return containerTypes_.contains(getType()); }

bool prm::Template::isMultiParm() const { return multiParmTypes_.contains(getType()); }

} // namespace enzo
