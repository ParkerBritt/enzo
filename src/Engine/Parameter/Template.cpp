#include "Engine/Parameter/Template.h"
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <utility>

namespace enzo
{

prm::Template::Template(prm::Type type, prm::Name name)
: Template(type, name, prm::Default())
{
    ranges_.resize(vectorSize_);
}

prm::Template::Template(prm::Type type, prm::Name name, unsigned int vectorSize)
: Template(type, name, prm::Default(), vectorSize)
{
    ranges_.resize(vectorSize_);
}

prm::Template::Template(prm::Type type, prm::Name name, std::vector<prm::Default> defaults, unsigned int vectorSize, std::vector<prm::Range> ranges)
: type_{type}, defaults_{defaults}, name_{name}, vectorSize_(vectorSize)
{
    if(ranges.size()<vectorSize_)
    {
        if(ranges.size()>=1)
        {
            ranges.resize(vectorSize_, ranges[0]);
        }
        else
        {
            ranges.resize(vectorSize_);
        }
    }
    ranges_ = ranges;
}

prm::Template::Template(prm::Type type, prm::Name name, prm::Default theDefault, unsigned int vectorSize, Range range)
: type_{type}, name_{name}, vectorSize_(vectorSize)
{
    defaults_.resize(vectorSize_, theDefault);
    ranges_.resize(vectorSize_, range);
}

const prm::Type prm::Template::getType() const
{
    return type_;
}

const unsigned int prm::Template::getSize() const
{
    return vectorSize_;
}

const prm::Default prm::Template::getDefault(unsigned int index) const
{
    return defaults_.at(index);
}

const prm::Range& prm::Template::getRange(unsigned int index) const
{
    return ranges_.at(index);
}

const unsigned int prm::Template::getNumDefaults() const
{
    return defaults_.size();
}

bt::String prm::Template::getName() const
{
    return name_.getToken();
}

bt::String prm::Template::getToken() const
{
    return name_.getToken();
}

bt::String prm::Template::getLabel() const
{
    return name_.getLabel();
}

prm::Direction prm::Template::getDirection() const
{
    return direction_;
}

const std::vector<prm::Template>& prm::Template::getChildren() const
{
    return children_;
}

bt::String prm::Template::getTooltip() const
{
    return tooltip_;
}

bt::String prm::Template::getDocumentation() const
{
    return documentation_;
}

bool prm::Template::isLabelHidden() const
{
    return labelHidden_;
}

prm::Template& prm::Template::setTooltip(bt::String tooltip)
{
    tooltip_ = std::move(tooltip);
    return *this;
}

prm::Template& prm::Template::setDocumentation(bt::String documentation)
{
    documentation_ = std::move(documentation);
    return *this;
}

prm::Template& prm::Template::setDirection(Direction direction)
{
    direction_ = direction;
    return *this;
}

prm::Template& prm::Template::addParm(Template child)
{
    children_.push_back(std::move(child));
    return *this;
}

prm::Template& prm::Template::setLabelHidden(bool hidden)
{
    labelHidden_ = hidden;
    return *this;
}

}
