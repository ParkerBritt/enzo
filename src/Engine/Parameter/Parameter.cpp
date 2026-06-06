#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/Default.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace enzo {

prm::Parameter::Parameter(Template prmTemplate) : template_{prmTemplate}
{
    const unsigned int size = prmTemplate.getSize();
    const unsigned int numDefaults = prmTemplate.getNumDefaults();

    auto getDefault = [&](unsigned int i) -> prm::Default {
        if (i < numDefaults) return prmTemplate.getDefault(i);
        if (numDefaults == 1) return prmTemplate.getDefault();
        return prm::Default();
    };

    switch (getType())
    {
    case prm::Type::FLOAT:
    case prm::Type::XYZ:
    {
        std::vector<floatT> vals(size);
        for (unsigned int i = 0; i < size; ++i)
            vals[i] = getDefault(i).getFloat();
        values_ = std::move(vals);
        break;
    }
    case prm::Type::INT:
    case prm::Type::BOOL:
    case prm::Type::TOGGLE:
    {
        std::vector<intT> vals(size);
        for (unsigned int i = 0; i < size; ++i)
            vals[i] = getDefault(i).getInt();
        values_ = std::move(vals);
        break;
    }
    case prm::Type::STRING:
    case prm::Type::DROPDOWN:
    {
        std::vector<String> vals(size);
        for (unsigned int i = 0; i < size; ++i)
            vals[i] = getDefault(i).getString();
        values_ = std::move(vals);
        break;
    }
    default:
        values_ = std::vector<floatT>(size, 0.0);
        break;
    }

    std::cout << "created new parameter: " << prmTemplate.getName() << "\n";
}

std::string prm::Parameter::getName() const { return template_.getName(); }

std::string prm::Parameter::getLabel() const { return template_.getLabel(); }

floatT prm::Parameter::evalFloat(unsigned int index) const
{
    auto& vals = std::get<std::vector<floatT>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    return vals[index];
}

intT prm::Parameter::evalInt(unsigned int index) const
{
    auto& vals = std::get<std::vector<intT>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    return vals[index];
}

String prm::Parameter::evalString(unsigned int index) const
{
    auto& vals = std::get<std::vector<String>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    return vals[index];
}

std::vector<floatT> prm::Parameter::evalFloats() const
{
    return std::get<std::vector<floatT>>(values_);
}

std::vector<String> prm::Parameter::evalStrings() const
{
    return std::get<std::vector<String>>(values_);
}

std::vector<intT> prm::Parameter::evalInts() const
{
    return std::get<std::vector<intT>>(values_);
}

unsigned int prm::Parameter::getVectorSize() const { return template_.getSize(); }

const prm::Template& prm::Parameter::getTemplate() { return template_; }

prm::Type prm::Parameter::getType() const { return template_.getType(); }

prm::ValueType prm::Parameter::getValueType() const
{
    switch (getType())
    {
    case prm::Type::FLOAT:
    case prm::Type::XYZ:
        return prm::ValueType::Float;
    case prm::Type::INT:
    case prm::Type::BOOL:
    case prm::Type::TOGGLE:
        return prm::ValueType::Int;
    case prm::Type::STRING:
    case prm::Type::DROPDOWN:
        return prm::ValueType::String;
    case prm::Type::GROUP:
        return prm::ValueType::Float;
    }
    return prm::ValueType::Float;
}

void prm::Parameter::setInt(intT value, unsigned int index)
{
    auto& vals = std::get<std::vector<intT>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    vals[index] = value;
    handleValueChange_();
}

void prm::Parameter::setFloat(floatT value, unsigned int index)
{
    auto& vals = std::get<std::vector<floatT>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    PrmValues before = vals;
    vals[index] = value;

    onFloatSet_(before);
    handleValueChange_();
}

void prm::Parameter::setString(String value, unsigned int index)
{
    auto& vals = std::get<std::vector<String>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    vals[index] = value;
    handleValueChange_();
}

prm::PrmValues prm::Parameter::getValues() const { return values_; }

void prm::Parameter::setValues(const PrmValues& values)
{
    values_ = values;
    handleValueChange_();
}

void prm::Parameter::handleValueChange_() { valueChanged(); }

} // namespace enzo
