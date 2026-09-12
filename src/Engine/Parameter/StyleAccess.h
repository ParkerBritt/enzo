#pragma once

/// @file
/// Reads and checks a parameter style and the values its options hold.
/// The styles themselves are in Styles.h.

#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/Styles.h"
#include "Engine/Parameter/Template.h"

#include <any>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace enzo::prm::style {

/// @brief Returns the names in a option written as a space separated list.
///
/// splitNames("point vertex") gives {"point", "vertex"}.
inline std::vector<std::string> splitNames(const std::string& text)
{
    std::istringstream stream(text);
    std::vector<std::string> names;
    for (std::string name; stream >> name;)
        names.push_back(name);
    return names;
}

/// @brief Returns the attribute owners a option names, e.g. "point vertex".
/// @note The name "all" stands for every owner. An unknown name throws.
inline std::vector<attr::AttributeOwner> parseOwners(const std::string& text)
{
    std::vector<attr::AttributeOwner> owners;
    for (const std::string& name : splitNames(text))
    {
        if (name == "all") return attr::getAllOwners();

        const std::optional<attr::AttributeOwner> owner = attr::getOwner(name);
        if (!owner) throw std::runtime_error("unknown attribute owner " + name);

        owners.push_back(*owner);
    }
    return owners;
}

/// @brief Returns the attribute types a option names, e.g. "float vector".
/// @note The name "all" stands for every type. An unknown name throws.
inline std::vector<attr::AttributeType> parseAttributeTypes(const std::string& text)
{
    std::vector<attr::AttributeType> types;
    for (const std::string& name : splitNames(text))
    {
        if (name == "all") return attr::getAllTypes();

        const std::optional<attr::AttributeType> type = attr::getType(name);
        if (!type) throw std::runtime_error("unknown attribute type " + name);

        types.push_back(*type);
    }
    return types;
}

/// @brief Returns the name of the parameter a option reads its value from.
///
/// getReferencedParameterName("prm(attachTo)") gives "attachTo", while a literal
/// such as "point vertex" gives nothing.
inline std::optional<std::string> getReferencedParameterName(const std::string& text)
{
    const std::vector<std::string> words = splitNames(text);
    if (words.size() != 1) return std::nullopt;

    const std::string& word = words.front();
    const std::string call = "prm(";
    if (!word.starts_with(call) || !word.ends_with(")")) return std::nullopt;

    const std::string name = word.substr(call.size(), word.size() - call.size() - 1);
    if (name.empty()) return std::nullopt;

    return name;
}

/// @brief Attaches the style a node.yaml name asks for.
/// @note An unknown name throws.
inline void attachStyle(prm::Template& parameter, const std::string& styleName)
{
    if (styleName == "boolSwitch")
        parameter.setStyle(BoolSwitch{});
    else if (styleName == "boolIcon")
        parameter.setStyle(BoolIcon{});
    else if (styleName == "xyz")
        parameter.setStyle(Xyz{});
    else if (styleName == "rangeCircle")
        parameter.setStyle(RangeCircle{});
    else if (styleName == "attribute")
        parameter.setStyle(Attribute{});
    else
        throw std::runtime_error("unknown style " + styleName);
}

/// @brief Returns whether a style is of a particular kind.
template <typename T> bool holds(const std::any& style)
{
    return std::any_cast<std::shared_ptr<T>>(&style) != nullptr;
}

/// @brief Returns the name a style is written under in node.yaml, e.g. "boolIcon".
/// @return The name, or empty when there is no style.
inline std::string toString(const std::any& style)
{
    if (holds<BoolSwitch>(style)) return "boolSwitch";
    if (holds<BoolIcon>(style)) return "boolIcon";
    if (holds<Xyz>(style)) return "xyz";
    if (holds<RangeCircle>(style)) return "rangeCircle";
    if (holds<Attribute>(style)) return "attribute";
    return "";
}

/// @brief Returns the options a style exposes to node.yaml.
/// @return The options, empty when the style takes none.
/// @note The options stay shared with the style, so writing one lands on the
/// style itself.
inline std::vector<std::shared_ptr<prm::Parameter>> options(const std::any& style)
{
    if (const auto* boolIcon = std::any_cast<std::shared_ptr<BoolIcon>>(&style))
        return (*boolIcon)->options;
    if (const auto* rangeCircle = std::any_cast<std::shared_ptr<RangeCircle>>(&style))
        return (*rangeCircle)->options;
    if (const auto* attribute = std::any_cast<std::shared_ptr<Attribute>>(&style))
        return (*attribute)->options;
    return {};
}

/// @brief Checks the value a style option holds.
///
/// An option written as prm(name) is checked against every choice the dropdown
/// it names offers.
///
/// @note A value the option cannot read, or a reference to a parameter the node
/// does not have, throws.
template <typename Parse>
void validateOption(
    const std::string& optionValue,
    const std::vector<const prm::Template*>& parameters,
    Parse parseValue
)
{
    const std::optional<std::string> name = getReferencedParameterName(optionValue);
    if (!name)
    {
        parseValue(optionValue);
        return;
    }

    for (const prm::Template* parameter : parameters)
    {
        if (parameter->getName() != *name) continue;

        for (const prm::Name& choice : parameter->getOptions())
            parseValue(choice.getToken());
        return;
    }

    throw std::runtime_error("no parameter named " + *name + " to read a style option from");
}

/// @brief Checks the values a style's options hold.
/// @param parameters Every parameter on the node holding the style.
inline void
validateOptions(const std::any& style, const std::vector<const prm::Template*>& parameters)
{
    const auto* attribute = std::any_cast<std::shared_ptr<Attribute>>(&style);
    if (!attribute) return;

    validateOption((*attribute)->owners(), parameters, parseOwners);
    validateOption((*attribute)->attributeTypes(), parameters, parseAttributeTypes);
}

/// @brief Returns the option a style exposes under a name, e.g. "scale".
/// @return The option, or null when the style has none by that name.
inline std::shared_ptr<prm::Parameter> getOption(const std::any& style, const std::string& name)
{
    for (const std::shared_ptr<prm::Parameter>& option : options(style))
        if (option->getName() == name) return option;
    return nullptr;
}

} // namespace enzo::prm::style
