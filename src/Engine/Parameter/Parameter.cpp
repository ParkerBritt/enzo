#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/Default.h"
#include "Engine/Types.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

enzo::prm::Parameter::Parameter(Template prmTemplate)
: template_{prmTemplate}
{
    const unsigned int size = prmTemplate.getSize();
    const unsigned int numDefaults = prmTemplate.getNumDefaults();

    auto getDefault = [&](unsigned int i) -> prm::Default {
        if(i < numDefaults) return prmTemplate.getDefault(i);
        if(numDefaults == 1) return prmTemplate.getDefault();
        return prm::Default();
    };

    switch(getType())
    {
        case prm::Type::FLOAT:
        case prm::Type::XYZ:
        {
            std::vector<bt::floatT> vals(size);
            for(unsigned int i = 0; i < size; ++i)
                vals[i] = getDefault(i).getFloat();
            values_ = std::move(vals);
            break;
        }
        case prm::Type::INT:
        case prm::Type::BOOL:
        case prm::Type::TOGGLE:
        {
            std::vector<bt::intT> vals(size);
            for(unsigned int i = 0; i < size; ++i)
                vals[i] = getDefault(i).getInt();
            values_ = std::move(vals);
            break;
        }
        case prm::Type::STRING:
        {
            std::vector<bt::String> vals(size);
            for(unsigned int i = 0; i < size; ++i)
                vals[i] = getDefault(i).getString();
            values_ = std::move(vals);
            break;
        }
        default:
            values_ = std::vector<bt::floatT>(size, 0.0);
            break;
    }

    std::cout << "created new parameter: " << prmTemplate.getName() << "\n";
}

std::string enzo::prm::Parameter::getName() const
{
    return template_.getName();
}

std::string enzo::prm::Parameter::getLabel() const
{
    return template_.getLabel();
}

enzo::bt::floatT enzo::prm::Parameter::evalFloat(unsigned int index) const
{
    auto& vals = std::get<std::vector<bt::floatT>>(values_);
    if(index >= vals.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    return vals[index];
}

enzo::bt::intT enzo::prm::Parameter::evalInt(unsigned int index) const
{
    auto& vals = std::get<std::vector<bt::intT>>(values_);
    if(index >= vals.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    return vals[index];
}

enzo::bt::String enzo::prm::Parameter::evalString(unsigned int index) const
{
    auto& vals = std::get<std::vector<bt::String>>(values_);
    if(index >= vals.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    return vals[index];
}

unsigned int enzo::prm::Parameter::getVectorSize() const
{
    return template_.getSize();
}


const enzo::prm::Template& enzo::prm::Parameter::getTemplate()
{
    return template_;
}



enzo::prm::Type enzo::prm::Parameter::getType() const
{
    return template_.getType();
}


void enzo::prm::Parameter::setInt(bt::intT value, unsigned int index)
{
    auto& vals = std::get<std::vector<bt::intT>>(values_);
    if(index >= vals.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    vals[index] = value;
    valueChanged();
}

void enzo::prm::Parameter::setFloat(bt::floatT value, unsigned int index)
{
    auto& vals = std::get<std::vector<bt::floatT>>(values_);
    if(index >= vals.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    vals[index] = value;
    valueChanged();
}

void enzo::prm::Parameter::setString(bt::String value, unsigned int index)
{
    auto& vals = std::get<std::vector<bt::String>>(values_);
    if(index >= vals.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    vals[index] = value;
    valueChanged();
}

enzo::prm::PrmValues enzo::prm::Parameter::getValues() const
{
    return values_;
}

void enzo::prm::Parameter::setValues(const PrmValues& values)
{
    values_ = values;
    valueChanged();
}
