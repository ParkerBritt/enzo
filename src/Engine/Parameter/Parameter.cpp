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
    const unsigned int templateSize = prmTemplate.getSize();
    const unsigned int numDefaults = prmTemplate.getNumDefaults();

    floatValues_.reserve(templateSize);
    stringValues_.reserve(templateSize);
    intValues_.reserve(templateSize);


    if(numDefaults==1)
    {
        floatValues_ = std::vector<bt::floatT>(templateSize, prmTemplate.getDefault().getFloat());
        intValues_ = std::vector<bt::intT>(templateSize, prmTemplate.getDefault().getInt());
        stringValues_ = std::vector<bt::String>(templateSize, prmTemplate.getDefault().getString());
    }

    for(int i=0; i<templateSize; ++i)
    {
        prm::Default prmDefault;
        if(i<numDefaults)
        {
             prmDefault = prmTemplate.getDefault(i);
        }
        floatValues_.push_back(prmDefault.getFloat());
        stringValues_.push_back(prmDefault.getString());
        intValues_.push_back(prmDefault.getInt());
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
    if(index >= floatValues_.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    return floatValues_[index];
}

enzo::bt::intT enzo::prm::Parameter::evalInt(unsigned int index) const
{
    if(index >= intValues_.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    return intValues_[index];
}

enzo::bt::String enzo::prm::Parameter::evalString(unsigned int index) const
{
    if(index >= stringValues_.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    return stringValues_[index];
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
    if(index >= intValues_.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    intValues_[index] = value;
    valueChanged();
}

void enzo::prm::Parameter::setFloat(bt::floatT value, unsigned int index)
{
    if(index >= floatValues_.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    floatValues_[index] = value;
    valueChanged();
}

void enzo::prm::Parameter::setString(bt::String value, unsigned int index)
{
    if(index >= stringValues_.size())
        throw std::out_of_range("Cannot access index: " + std::to_string(index) + " for parameter: " + getName());
    stringValues_[index] = value;
    valueChanged();
}
