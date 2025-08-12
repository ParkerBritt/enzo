#include "Engine/Parameter/Template.h"
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"
#include <icecream.hpp>

enzo::prm::Template::Template(enzo::prm::Type type, prm::Name name)
: Template(type, name, prm::Default())
{
    ranges_.resize(vectorSize_);
}

enzo::prm::Template::Template(enzo::prm::Type type, prm::Name name, unsigned int vectorSize)
: Template(type, name, prm::Default(), vectorSize)
{
    ranges_.resize(vectorSize_);
}

enzo::prm::Template::Template(enzo::prm::Type type, prm::Name name, std::vector<prm::Default> defaults, unsigned int vectorSize, std::vector<prm::Range> ranges)
: type_{type}, name_{name}, defaults_{defaults}, vectorSize_(vectorSize)
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

enzo::prm::Template::Template(enzo::prm::Type type, prm::Name name, prm::Default theDefault, unsigned int vectorSize, Range range)
: type_{type}, name_{name}, vectorSize_(vectorSize)
{
    defaults_.resize(vectorSize_, theDefault);
    ranges_.resize(vectorSize_, range);
}

enzo::prm::Template::Template()
: type_{enzo::prm::Type::LIST_TERMINATOR}
{
    
}

// bool enzo::prm::Template::isValid() const
// {
//     return name_!=nullptr;

// }

const enzo::prm::Type enzo::prm::Template::getType() const
{
    return type_;
}

const unsigned int enzo::prm::Template::getSize() const
{
    return vectorSize_;
}



const enzo::prm::Default enzo::prm::Template::getDefault(unsigned int index) const
{
    return defaults_.at(index);
}

const enzo::prm::Range& enzo::prm::Template::getRange(unsigned int index) const
{
    return ranges_.at(index);
}

const unsigned int enzo::prm::Template::getNumDefaults() const
{
    return defaults_.size();
}

enzo::bt::String enzo::prm::Template::getName() const
{
    return name_.getToken();
}

enzo::bt::String enzo::prm::Template::getToken() const
{
    return name_.getToken();
}

enzo::bt::String enzo::prm::Template::getLabel() const
{
    return name_.getLabel();
}

