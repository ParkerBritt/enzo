#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Template.h"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace enzo::prm::style {

struct BoolSwitch
{
};

// A vector whose components read as x, y and z axes, each with its own colour.
struct Xyz
{
};

// A pair of floats read as the start and end of an arc, drawn as a dial.
struct RangeCircle
{
    enum Setting
    {
        ORDERED,
        CAPTION
    };

    std::vector<std::shared_ptr<prm::Parameter>> settings = {
        std::make_shared<prm::Parameter>(
            Template(Type::BOOL, Name("ordered", "Ordered"), Default(false))
        ),
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("caption", "Caption"), Default(""))
        ),
    };

    // Whether the low bound stays below the high one rather than the arc
    // wrapping past the end of the range.
    bool ordered() const { return settings[ORDERED]->evalInt() != 0; }

    // The label shown with the arc's reading, e.g. "arc", empty for none.
    String caption() const { return settings[CAPTION]->evalString(); }
};

// A bool drawn as an icon rather than a checkbox.
struct BoolIcon
{
    enum Setting
    {
        ICON,
        SCALE
    };

    std::vector<std::shared_ptr<prm::Parameter>> settings = {
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("icon", "Icon"), Default("eye"))
        ),
        std::make_shared<prm::Parameter>(
            Template(Type::FLOAT, Name("scale", "Scale"), Default(0.95f))
        ),
    };

    String icon() const { return settings[ICON]->evalString(); }
    floatT scale() const { return settings[SCALE]->evalFloat(); }
};

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
    return {};
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
