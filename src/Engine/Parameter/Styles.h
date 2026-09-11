#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Template.h"

#include <memory>
#include <optional>
#include <sstream>
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

/// @brief Returns the names in a setting written as a space separated list.
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

/// @brief Returns the attribute owners a setting names, e.g. "point vertex".
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

/// @brief Returns the attribute types a setting names, e.g. "float vector".
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

// A string naming a geometry attribute, with a list of the attributes on the
// node's input to pick from.
struct Attribute
{
    enum Setting
    {
        OWNERS,
        ATTRIBUTE_TYPES
    };

    std::vector<std::shared_ptr<prm::Parameter>> settings = {
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("owners", "Owners"), Default("all"))
        ),
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("attributeTypes", "Attribute Types"), Default("all"))
        ),
    };

    /// @brief Returns the parts of the geometry the attributes come from.
    /// @note An unknown name throws.
    std::vector<attr::AttributeOwner> getOwners() const
    {
        return parseOwners(settings[OWNERS]->evalString());
    }

    /// @brief Returns the value types the attributes can hold.
    /// @note An unknown name throws.
    std::vector<attr::AttributeType> getTypes() const
    {
        return parseAttributeTypes(settings[ATTRIBUTE_TYPES]->evalString());
    }
};

} // namespace enzo::prm::style
