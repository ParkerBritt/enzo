#pragma once

/// @file
/// The parameter styles a gui can use.
///
/// Each style is a plain struct holding its options as parameters. Reading and
/// checking a style is in StyleAccess.h.

#include "Engine/Core/Types.h"
#include "Engine/Parameter/Parameter.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Template.h"

#include <memory>
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
    enum Option
    {
        ORDERED,
        CAPTION
    };

    std::vector<std::shared_ptr<prm::Parameter>> options = {
        std::make_shared<prm::Parameter>(
            Template(Type::BOOL, Name("ordered", "Ordered"), Default(false))
        ),
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("caption", "Caption"), Default(""))
        ),
    };

    // Whether the low bound stays below the high one rather than the arc
    // wrapping past the end of the range.
    bool ordered() const { return options[ORDERED]->evalInt() != 0; }

    // The label shown with the arc's reading, e.g. "arc", empty for none.
    String caption() const { return options[CAPTION]->evalString(); }
};

// A bool drawn as an icon rather than a checkbox.
struct BoolIcon
{
    enum Option
    {
        ICON,
        SCALE
    };

    std::vector<std::shared_ptr<prm::Parameter>> options = {
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("icon", "Icon"), Default("eye"))
        ),
        std::make_shared<prm::Parameter>(
            Template(Type::FLOAT, Name("scale", "Scale"), Default(0.95f))
        ),
    };

    String icon() const { return options[ICON]->evalString(); }
    floatT scale() const { return options[SCALE]->evalFloat(); }
};

// A string naming a geometry attribute, with a list of the attributes on the
// node's input to pick from.
struct Attribute
{
    enum Option
    {
        OWNERS,
        ATTRIBUTE_TYPES
    };

    std::vector<std::shared_ptr<prm::Parameter>> options = {
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("owners", "Owners"), Default("all"))
        ),
        std::make_shared<prm::Parameter>(
            Template(Type::STRING, Name("attributeTypes", "Attribute Types"), Default("all"))
        ),
    };

    /// @brief Returns the parts of the geometry the attributes come from, e.g. "point vertex".
    String owners() const { return options[OWNERS]->evalString(); }

    /// @brief Returns the value types the attributes can hold, e.g. "float vector".
    String attributeTypes() const { return options[ATTRIBUTE_TYPES]->evalString(); }
};

} // namespace enzo::prm::style
