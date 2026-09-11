#pragma once

#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/Styles.h"
#include "Engine/Parameter/Template.h"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace enzo::prm::style {

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

/// @brief Returns the settings a style exposes to node.yaml.
/// @return The settings, empty when the style takes none.
/// @note The settings stay shared with the style, so writing one lands on the
/// style itself.
inline std::vector<std::shared_ptr<prm::Parameter>> settings(const std::any& style)
{
    if (const auto* boolIcon = std::any_cast<std::shared_ptr<BoolIcon>>(&style))
        return (*boolIcon)->settings;
    if (const auto* rangeCircle = std::any_cast<std::shared_ptr<RangeCircle>>(&style))
        return (*rangeCircle)->settings;
    if (const auto* attribute = std::any_cast<std::shared_ptr<Attribute>>(&style))
        return (*attribute)->settings;
    return {};
}

/// @brief Checks the values a style's settings hold.
/// @note A value the style cannot read throws.
inline void validateSettings(const std::any& style)
{
    if (const auto* attribute = std::any_cast<std::shared_ptr<Attribute>>(&style))
    {
        (*attribute)->getOwners();
        (*attribute)->getTypes();
    }
}

/// @brief Returns the setting a style exposes under a name, e.g. "scale".
/// @return The setting, or null when the style has none by that name.
inline std::shared_ptr<prm::Parameter> getSetting(const std::any& style, const std::string& name)
{
    for (const std::shared_ptr<prm::Parameter>& setting : settings(style))
        if (setting->getName() == name) return setting;
    return nullptr;
}

} // namespace enzo::prm::style
