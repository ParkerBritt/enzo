#include "Engine/Network/NodeManifest.h"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Parameter/Style.h"
#include "Engine/Parameter/Template.h"

#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace enzo::nt {

namespace {

// The schema this parser understands. A manifest claiming any other version is
// rejected rather than guessed at.
constexpr int kSchemaVersion = 1;

std::string readString(const YAML::Node& parent, const std::string& key, std::string fallback = "")
{
    const YAML::Node value = parent[key];
    return value ? value.as<std::string>() : std::move(fallback);
}

std::string requireString(const YAML::Node& parent, const std::string& key, const std::string& what)
{
    const YAML::Node value = parent[key];
    if (!value) throw std::runtime_error(what + " has no " + key);
    return value.as<std::string>();
}

prm::Default readDefault(const YAML::Node& value, prm::ValueType valueType)
{
    if (!value) return prm::Default();

    switch (valueType)
    {
    case prm::ValueType::String:
        return prm::Default(value.as<std::string>().c_str());
    case prm::ValueType::Int:
    {
        // A bool parameter stores an int, so it may be written either way.
        bool boolValue = false;
        if (YAML::convert<bool>::decode(value, boolValue)) return prm::Default(boolValue);
        return prm::Default(value.as<int>());
    }
    case prm::ValueType::Float:
        return prm::Default(value.as<floatT>());
    }
    return prm::Default();
}

prm::Range readRange(const YAML::Node& range)
{
    if (!range) return prm::Range();

    const auto lockFlag = [&](const std::string& key) {
        const bool locked = range[key] && range[key].as<bool>();
        return locked ? prm::RangeFlag::LOCKED : prm::RangeFlag::UNLOCKED;
    };

    return prm::Range(
        range["min"] ? range["min"].as<floatT>() : 0,
        range["max"] ? range["max"].as<floatT>() : 10,
        lockFlag("minLocked"),
        lockFlag("maxLocked")
    );
}

std::vector<prm::Name> readOptions(const YAML::Node& options)
{
    std::vector<prm::Name> names;
    for (const YAML::Node& option : options)
    {
        const std::string value = requireString(option, "value", "option");
        names.emplace_back(value, readString(option, "label", value));
    }
    return names;
}

prm::Direction readDirection(const YAML::Node& direction)
{
    const std::string text = direction.as<std::string>();
    if (text == "horizontal") return prm::Direction::HORIZONTAL;
    if (text == "vertical") return prm::Direction::VERTICAL;
    throw std::runtime_error("unknown direction " + text);
}

// Attaches one of the style structs named in Style.h. The two icon styles take
// the same settings.
void readStyle(prm::Template& parameter, const YAML::Node& style)
{
    const std::string kind = requireString(style, "kind", "style");

    const auto attach = [&](auto shape) {
        if (style["icon"]) shape.setIcon(style["icon"].as<std::string>());
        if (style["scale"]) shape.setScale(style["scale"].as<floatT>());
        parameter.setStyle(std::move(shape));
    };

    if (kind == "boolSwitch")
        parameter.setStyle(prm::style::BoolSwitch{});
    else if (kind == "boolIcon")
        attach(prm::style::BoolIcon{});
    else if (kind == "boolIconSlash")
        attach(prm::style::BoolIconSlash{});
    else
        throw std::runtime_error("unknown style " + kind);
}

// Reads the starting values of a multiparm, one list per field of its instance
// template. A ramp names its "position" and "value" fields here.
void readInstanceDefaults(prm::Template& parameter, const YAML::Node& fields)
{
    for (const auto& field : fields)
    {
        const std::string fieldToken = field.first.as<std::string>();
        const prm::ValueType valueType = prm::toValueType(parameter.getChild(fieldToken).getType());

        std::vector<prm::Default> defaults;
        for (const YAML::Node& value : field.second)
            defaults.push_back(readDefault(value, valueType));

        parameter.setInstanceDefault(fieldToken, std::move(defaults));
    }
}

prm::Template readParameter(const YAML::Node& parm)
{
    const std::string name = requireString(parm, "name", "parameter");
    const std::string typeName = requireString(parm, "type", "parameter " + name);
    const std::optional<prm::Type> type = prm::toType(typeName);
    if (!type) throw std::runtime_error("parameter " + name + " has unknown type " + typeName);

    const prm::Name parmName(name, readString(parm, "label", name));
    const prm::ValueType valueType = prm::toValueType(*type);
    const unsigned int size = parm["size"] ? parm["size"].as<unsigned int>() : 1;
    const prm::Range range = readRange(parm["range"]);

    // A list of defaults gives every component of a vector its own value, while
    // a single one covers them all.
    const YAML::Node defaultValue = parm["default"];
    const bool isPerComponent = defaultValue && defaultValue.IsSequence();

    std::vector<prm::Default> componentDefaults;
    if (isPerComponent)
        for (const YAML::Node& value : defaultValue)
            componentDefaults.push_back(readDefault(value, valueType));

    prm::Template parameter =
        isPerComponent
            ? prm::Template(*type, parmName, componentDefaults, size, {range})
            : prm::Template(*type, parmName, readDefault(defaultValue, valueType), size, range);

    if (parm["tooltip"]) parameter.setTooltip(parm["tooltip"].as<std::string>());
    if (parm["documentation"]) parameter.setDocumentation(parm["documentation"].as<std::string>());
    if (parm["disableCondition"])
        parameter.setDisableCondition(parm["disableCondition"].as<std::string>());
    if (parm["hideCondition"]) parameter.setHideCondition(parm["hideCondition"].as<std::string>());
    if (parm["direction"]) parameter.setDirection(readDirection(parm["direction"]));
    if (parm["labelHidden"]) parameter.setLabelHidden(parm["labelHidden"].as<bool>());
    if (parm["background"]) parameter.setBackgroundEnabled(parm["background"].as<bool>());
    if (parm["options"]) parameter.setOptions(readOptions(parm["options"]));
    if (parm["style"]) readStyle(parameter, parm["style"]);
    if (parm["instanceDefaults"]) readInstanceDefaults(parameter, parm["instanceDefaults"]);

    for (const YAML::Node& child : parm["parameters"])
        parameter.addParm(readParameter(child));

    return parameter;
}

std::vector<prm::Template> readParameters(const YAML::Node& parameters)
{
    std::vector<prm::Template> templates;
    for (const YAML::Node& parm : parameters)
        templates.push_back(readParameter(parm));
    return templates;
}

NodeImplementation readImplementation(const YAML::Node& implementation, const std::string& nodeName)
{
    if (!implementation) throw std::runtime_error("node " + nodeName + " has no implementation");

    NodeImplementation parsed;
    parsed.kind = requireString(implementation, "kind", "implementation");
    if (parsed.kind != "cpp")
        throw std::runtime_error("implementation kind " + parsed.kind + " is not supported yet");

    parsed.library = requireString(implementation, "library", "implementation");
    parsed.constructor = readString(implementation, "constructor", nodeName);
    return parsed;
}

std::vector<std::string> readTags(const YAML::Node& tags)
{
    std::vector<std::string> parsed;
    for (const YAML::Node& tag : tags)
        parsed.push_back(tag.as<std::string>());
    return parsed;
}

} // namespace

NodeManifest NodeManifest::loadFromString(const std::string& yaml)
{
    const YAML::Node document = YAML::Load(yaml);

    const int version = document["version"] ? document["version"].as<int>() : 0;
    if (version != kSchemaVersion)
        throw std::runtime_error(
            "node manifest version " + std::to_string(version) + " is not supported"
        );

    NodeManifest manifest;

    NodeType& nodeType = manifest.nodeType_;
    nodeType.internalName = requireString(document, "name", "node manifest");
    nodeType.typeNamespace = requireString(document, "namespace", "node manifest");
    nodeType.displayName = readString(document, "label", nodeType.internalName);
    nodeType.templates = readParameters(document["parameters"]);
    nodeType.tags = readTags(document["tags"]);
    nodeType.iconPath = readString(document, "icon");
    nodeType.docsPath = readString(document, "docs");

    // Counts left out of the manifest keep the one input one output shape a
    // NodeType starts with.
    const YAML::Node inputs = document["inputs"];
    if (inputs && inputs["min"]) nodeType.minInputs = inputs["min"].as<unsigned int>();
    if (inputs && inputs["max"]) nodeType.maxInputs = inputs["max"].as<unsigned int>();
    if (document["outputs"]) nodeType.maxOutputs = document["outputs"].as<unsigned int>();

    manifest.implementation_ =
        readImplementation(document["implementation"], nodeType.internalName);

    return manifest;
}

NodeManifest NodeManifest::loadFromFile(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        throw std::runtime_error("no node manifest at " + path.string());

    std::ifstream file(path);
    const std::string yaml(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // The file name goes on the front of whatever went wrong, since the parser
    // itself only ever sees a document.
    try
    {
        return loadFromString(yaml);
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error(path.string() + " " + error.what());
    }
}

} // namespace enzo::nt
