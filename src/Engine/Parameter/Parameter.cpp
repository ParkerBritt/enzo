#include "Engine/Parameter/Parameter.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Expression/ExpressionEngine.h"
#include "Engine/Parameter/Default.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace enzo {

prm::Parameter::Parameter(Template prmTemplate) : template_{prmTemplate}
{
    // Multiparm parameters such as ramps hold a dynamic list of instances rather
    // than a flat value. The parent default carries the initial instance count.
    if (template_.isMultiParm())
    {
        const unsigned int initialCount = template_.getDefault().getInt();
        for (unsigned int instanceIndex = 0; instanceIndex < initialCount; ++instanceIndex)
            instances_.push_back(buildInstance_(instanceIndex));
        return;
    }

    const unsigned int size = prmTemplate.getSize();
    const unsigned int numDefaults = prmTemplate.getNumDefaults();

    expressions_.resize(size);

    auto getDefault = [&](unsigned int i) -> prm::Default {
        if (i < numDefaults) return prmTemplate.getDefault(i);
        if (numDefaults == 1) return prmTemplate.getDefault();
        return prm::Default();
    };

    switch (getValueType())
    {
    case prm::ValueType::Float:
    {
        std::vector<floatT> vals(size);
        for (unsigned int i = 0; i < size; ++i)
            vals[i] = getDefault(i).getFloat();
        values_ = std::move(vals);
        break;
    }
    case prm::ValueType::Int:
    {
        std::vector<intT> vals(size);
        for (unsigned int i = 0; i < size; ++i)
            vals[i] = getDefault(i).getInt();
        values_ = std::move(vals);
        break;
    }
    case prm::ValueType::String:
    {
        std::vector<String> vals(size);
        for (unsigned int i = 0; i < size; ++i)
            vals[i] = getDefault(i).getString();
        values_ = std::move(vals);
        break;
    }
    }

    std::cout << "created new parameter: " << prmTemplate.getName() << "\n";
}

std::string prm::Parameter::getName() const { return template_.getName(); }

std::string prm::Parameter::getLabel() const { return template_.getLabel(); }

floatT prm::Parameter::evalFloat(unsigned int index) const
{
    String error;
    return evalFloat(index, error);
}

floatT prm::Parameter::evalFloat(unsigned int index, String& error) const
{
    error.clear();

    // An expression drives the value, yielding zero when it fails to compile or
    // run.
    if (hasExpression(index))
    {
        auto context = makeExpressionContext_();
        floatT result = 0;
        const bool runSuccessful =
            expr::ExpressionEngine::instance()
                .evalFloat(*expressions_[index], context.get(), result, error);
        if (context) submitExpressionDependencies_(*context, index);
        if (!runSuccessful) return 0;
        return clampToRange_(result, index);
    }

    return readFloatLiteral_(index);
}

intT prm::Parameter::evalInt(unsigned int index) const
{
    String error;
    return evalInt(index, error);
}

intT prm::Parameter::evalInt(unsigned int index, String& error) const
{
    error.clear();

    // Maps string tokens to integer indeces for parameters with options like dropdowns.
    if (template_.hasOptions())
    {
        const String token = evalString(index);
        const std::vector<prm::Name>& options = template_.getOptions();
        for (unsigned int optionIndex = 0; optionIndex < options.size(); ++optionIndex)
            if (options[optionIndex].getToken() == token) return optionIndex;
        return -1;
    }

    // An expression drives the value, yielding zero when it fails to compile or
    // run.
    if (hasExpression(index))
    {
        auto context = makeExpressionContext_();
        intT result = 0;
        const bool runSuccessful = expr::ExpressionEngine::instance()
                                       .evalInt(*expressions_[index], context.get(), result, error);
        if (context) submitExpressionDependencies_(*context, index);
        if (!runSuccessful) return 0;
        return clampToRange_(result, index);
    }

    return readIntLiteral_(index);
}

std::unique_ptr<expr::ExpressionContext> prm::Parameter::makeExpressionContext_() const
{
    return nullptr;
}

String prm::Parameter::evalString(unsigned int index) const
{
    String error;
    return evalString(index, error);
}

String prm::Parameter::evalString(unsigned int index, String& error) const
{
    error.clear();

    // An expression drives the value, yielding an empty string when it fails to
    // compile or run.
    if (hasExpression(index))
    {
        auto context = makeExpressionContext_();
        String result;
        const bool runSuccessful =
            expr::ExpressionEngine::instance()
                .evalString(*expressions_[index], context.get(), result, error);
        if (context) submitExpressionDependencies_(*context, index);
        if (!runSuccessful) return String();
        return result;
    }

    return readStringLiteral_(index);
}

// Evaluates each component, running any expression that drives it to produce
// the value.
std::vector<floatT> prm::Parameter::evalFloats() const
{
    const auto& vals = std::get<std::vector<floatT>>(values_);
    std::vector<floatT> results;
    results.reserve(vals.size());
    for (unsigned int index = 0; index < vals.size(); ++index)
        results.push_back(evalFloat(index));
    return results;
}

std::vector<String> prm::Parameter::evalStrings() const
{
    const auto& vals = std::get<std::vector<String>>(values_);
    std::vector<String> results;
    results.reserve(vals.size());
    for (unsigned int index = 0; index < vals.size(); ++index)
        results.push_back(evalString(index));
    return results;
}

std::vector<intT> prm::Parameter::evalInts() const
{
    const auto& vals = std::get<std::vector<intT>>(values_);
    std::vector<intT> results;
    results.reserve(vals.size());
    for (unsigned int index = 0; index < vals.size(); ++index)
        results.push_back(evalInt(index));
    return results;
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
    // Multiparm parameters (like ramp) use integers to represent their instance
    // count and store the actual data in their instances.
    case prm::Type::RAMP:
    // Spacers are purely visual and store an int nobody reads.
    case prm::Type::SPACER:
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
    // Maps integer indeces back to string tokens for parameters with options like dropdowns.
    if (template_.hasOptions())
    {
        const std::vector<prm::Name>& options = template_.getOptions();
        if (value < 0 || value >= static_cast<intT>(options.size()))
            throw std::out_of_range(
                "Option index: " + std::to_string(value) +
                " out of range for parameter: " + getName()
            );
        setString(options[value].getToken(), index);
        return;
    }

    auto& vals = std::get<std::vector<intT>>(values_);
    if (index >= vals.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    vals[index] = value;
    expressions_[index].reset();
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
    expressions_[index].reset();

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
    expressions_[index].reset();
    handleValueChange_();
}

void prm::Parameter::setExpression(String expression, unsigned int index)
{
    if (index >= expressions_.size())
        throw std::out_of_range(
            "Cannot access index: " + std::to_string(index) + " for parameter: " + getName()
        );
    expressions_[index] = std::move(expression);
    handleValueChange_();
}

void prm::Parameter::clearExpression(unsigned int index)
{
    if (index >= expressions_.size()) return;
    expressions_[index].reset();
    handleValueChange_();
}

bool prm::Parameter::hasExpression(unsigned int index) const
{
    return index < expressions_.size() && expressions_[index].has_value();
}

std::optional<String> prm::Parameter::getExpression(unsigned int index) const
{
    if (index >= expressions_.size()) return std::nullopt;
    return expressions_[index];
}

prm::PrmValues prm::Parameter::getValues() const { return values_; }

void prm::Parameter::setValues(const PrmValues& values)
{
    values_ = values;
    handleValueChange_();
}

floatT prm::Parameter::clampToRange_(floatT value, unsigned int index) const
{
    const prm::Range& range = template_.getRange(index);
    if (range.getMinFlag() == prm::RangeFlag::LOCKED && value < range.getMin())
        return range.getMin();
    if (range.getMaxFlag() == prm::RangeFlag::LOCKED && value > range.getMax())
        return range.getMax();
    return value;
}

intT prm::Parameter::clampToRange_(intT value, unsigned int index) const
{
    const prm::Range& range = template_.getRange(index);
    // Round the locked bounds inward so the result always sits inside the range.
    if (range.getMinFlag() == prm::RangeFlag::LOCKED && value < range.getMin())
        return static_cast<intT>(std::ceil(range.getMin()));
    if (range.getMaxFlag() == prm::RangeFlag::LOCKED && value > range.getMax())
        return static_cast<intT>(std::floor(range.getMax()));
    return value;
}

floatT prm::Parameter::readFloatLiteral_(unsigned int index) const
{
    if (const auto* floats = std::get_if<std::vector<floatT>>(&values_))
        return index < floats->size() ? (*floats)[index] : 0;
    if (const auto* ints = std::get_if<std::vector<intT>>(&values_))
        return index < ints->size() ? static_cast<floatT>((*ints)[index]) : 0;
    return 0;
}

intT prm::Parameter::readIntLiteral_(unsigned int index) const
{
    if (const auto* ints = std::get_if<std::vector<intT>>(&values_))
        return index < ints->size() ? (*ints)[index] : 0;
    if (const auto* floats = std::get_if<std::vector<floatT>>(&values_))
        return index < floats->size() ? static_cast<intT>((*floats)[index]) : 0;
    return 0;
}

String prm::Parameter::readStringLiteral_(unsigned int index) const
{
    if (const auto* strings = std::get_if<std::vector<String>>(&values_))
        return index < strings->size() ? (*strings)[index] : String();
    return String();
}

void prm::Parameter::handleValueChange_() { valueChanged(); }

std::vector<std::shared_ptr<prm::Parameter>>
prm::Parameter::buildInstance_(unsigned int instanceIndex)
{
    std::vector<std::shared_ptr<Parameter>> fields;
    for (const Template& fieldTemplate : template_.getChildren())
    {
        auto field = std::make_shared<Parameter>(fieldTemplate);

        // Seed the field from its per instance default when one is set, so each
        // control point can start at a distinct value.
        if (auto instanceDefault =
                template_.getInstanceDefault(fieldTemplate.getToken(), instanceIndex))
        {
            switch (field->getValueType())
            {
            case prm::ValueType::Float:
                field->setFloat(instanceDefault->getFloat());
                break;
            case prm::ValueType::Int:
                field->setInt(instanceDefault->getInt());
                break;
            case prm::ValueType::String:
                field->setString(instanceDefault->getString());
                break;
            }
        }

        // A field edit bubbles up so the owning node recooks.
        field->valueChanged.connect([this]() { handleValueChange_(); });
        fields.push_back(std::move(field));
    }
    return fields;
}

unsigned int prm::Parameter::getInstanceCount() const { return instances_.size(); }

const std::vector<std::shared_ptr<prm::Parameter>>&
prm::Parameter::getInstance(unsigned int instanceIndex) const
{
    return instances_.at(instanceIndex);
}

std::shared_ptr<prm::Parameter>
prm::Parameter::getInstanceField(unsigned int instanceIndex, std::string_view fieldName) const
{
    for (const std::shared_ptr<Parameter>& field : instances_.at(instanceIndex))
        if (field->getName() == fieldName) return field;
    return nullptr;
}

void prm::Parameter::addInstance()
{
    instances_.push_back(buildInstance_(instances_.size()));
    handleValueChange_();
}

void prm::Parameter::removeInstance(unsigned int instanceIndex)
{
    if (instanceIndex >= instances_.size())
        throw std::out_of_range(
            "Cannot remove instance " + std::to_string(instanceIndex) +
            " for parameter: " + getName()
        );
    instances_.erase(instances_.begin() + instanceIndex);
    handleValueChange_();
}

void prm::Parameter::moveInstance(unsigned int fromIndex, unsigned int toIndex)
{
    if (fromIndex >= instances_.size() || toIndex >= instances_.size())
        throw std::out_of_range("Cannot move instance for parameter: " + getName());
    std::vector<std::shared_ptr<Parameter>> moved = std::move(instances_[fromIndex]);
    instances_.erase(instances_.begin() + fromIndex);
    instances_.insert(instances_.begin() + toIndex, std::move(moved));
    handleValueChange_();
}

} // namespace enzo
