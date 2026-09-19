#include "Engine/Network/NodeManifest.h"
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Parameter/StyleAccess.h"
#include "Engine/Parameter/Template.h"

#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
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

// Returns the per component values of a list, or the single value that covers
// every component.
std::vector<YAML::Node> getComponents(const YAML::Node& value)
{
    if (!value.IsSequence()) return {value};
    return std::vector<YAML::Node>(value.begin(), value.end());
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

// Writes a value from node.yaml onto a style option.
void readStyleOption(prm::Parameter& option, const YAML::Node& value)
{
    const prm::Default parsed = readDefault(value, option.getValueType());

    switch (option.getValueType())
    {
    case prm::ValueType::String:
        option.setString(parsed.getString());
        return;
    case prm::ValueType::Int:
        option.setInt(parsed.getInt());
        return;
    case prm::ValueType::Float:
        option.setFloat(parsed.getFloat());
        return;
    }
}

// Attaches the style named in node.yaml and fills in the options it exposes
// from the styleOptions block. An unknown style or a option the style does not
// have throws.
void readStyle(prm::Template& parameter, const YAML::Node& style, const YAML::Node& styleOptions)
{
    const std::string styleName = style.as<std::string>();
    prm::style::attachStyle(parameter, styleName);

    if (!styleOptions) return;

    for (const auto& entry : styleOptions)
    {
        const std::string optionName = entry.first.as<std::string>();
        const std::shared_ptr<prm::Parameter> option =
            prm::style::getOption(parameter.getStyle(), optionName);

        if (!option)
            throw std::runtime_error("style " + styleName + " has no option " + optionName);

        readStyleOption(*option, entry.second);
    }
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

    prm::Template parameter(*type, parmName, prm::Default(), size, range);

    if (parm["default"])
    {
        std::vector<prm::Default> defaults;
        for (const YAML::Node& component : getComponents(parm["default"]))
            defaults.push_back(readDefault(component, valueType));
        parameter.setDefaults(std::move(defaults));
    }

    if (parm["tooltip"]) parameter.setTooltip(parm["tooltip"].as<std::string>());
    if (parm["documentation"]) parameter.setDocumentation(parm["documentation"].as<std::string>());
    if (parm["disableCondition"])
        parameter.setDisableCondition(parm["disableCondition"].as<std::string>());
    if (parm["hideCondition"]) parameter.setHideCondition(parm["hideCondition"].as<std::string>());
    if (parm["direction"]) parameter.setDirection(readDirection(parm["direction"]));
    if (parm["labelHidden"]) parameter.setLabelHidden(parm["labelHidden"].as<bool>());
    if (parm["labelInline"]) parameter.setLabelInline(parm["labelInline"].as<bool>());
    if (parm["background"]) parameter.setBackgroundEnabled(parm["background"].as<bool>());
    if (parm["icon"]) parameter.setIcon(parm["icon"].as<std::string>());
    if (parm["options"]) parameter.setOptions(readOptions(parm["options"]));
    if (parm["styleOptions"] && !parm["style"])
        throw std::runtime_error("parameter " + name + " has styleOptions but no style");
    if (parm["style"]) readStyle(parameter, parm["style"], parm["styleOptions"]);
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

// Returns every parameter on the node, the ones nested in groups included.
std::vector<const prm::Template*>
getFlattenedParameters(const std::vector<prm::Template>& templates)
{
    std::vector<const prm::Template*> parameters;
    for (const prm::Template& parameter : templates)
    {
        parameters.push_back(&parameter);

        const std::vector<const prm::Template*> children =
            getFlattenedParameters(parameter.getChildren());
        parameters.insert(parameters.end(), children.begin(), children.end());
    }
    return parameters;
}

// Returns the parameter carrying a name, including one nested in a group, or
// nullptr when none does.
const prm::Template*
getParameter(const std::vector<prm::Template>& templates, const std::string& name)
{
    for (const prm::Template& parameter : templates)
    {
        if (parameter.getName() == name) return &parameter;
        if (!parameter.isContainer()) continue;

        const prm::Template* child = getParameter(parameter.getChildren(), name);
        if (child) return child;
    }
    return nullptr;
}

// Checks the style options of every parameter. A value a style cannot read, or
// a missing parameter, throws.
void validateStyles(const std::vector<prm::Template>& templates)
{
    const std::vector<const prm::Template*> parameters = getFlattenedParameters(templates);
    for (const prm::Template* parameter : parameters)
        prm::style::validateOptions(parameter->getStyle(), parameters);
}

NodeImplementation readImplementation(const YAML::Node& implementation, const std::string& nodeName)
{
    if (!implementation) throw std::runtime_error("node " + nodeName + " has no implementation");

    const std::string kind = requireString(implementation, "kind", "implementation");

    if (kind == "cpp")
        return CppImplementation{
            .library = requireString(implementation, "library", "implementation"),
            .constructor = readString(implementation, "constructor", nodeName),
        };

    if (kind == "alias")
        return AliasImplementation{
            .aliasedType = requireString(implementation, "type", "alias implementation"),
        };

    throw std::runtime_error("implementation kind " + kind + " is not supported yet");
}

// Returns the parameter values of an alias. A single value becomes a list of one.
std::map<std::string, std::vector<std::string>> readParameterValues(const YAML::Node& values)
{
    std::map<std::string, std::vector<std::string>> parsed;
    for (const auto& entry : values)
    {
        std::vector<std::string>& values = parsed[entry.first.as<std::string>()];
        for (const YAML::Node& component : getComponents(entry.second))
            values.push_back(component.as<std::string>());
    }
    return parsed;
}

ParameterSerializable
readParameterValue(const prm::Template& parameter, const std::vector<std::string>& componentValues)
{
    const prm::ValueType valueType = prm::toValueType(parameter.getType());
    ParameterSerializable value;
    value.name = parameter.getName();

    for (unsigned int componentIndex = 0; componentIndex < parameter.getSize(); ++componentIndex)
    {
        const std::string& componentText =
            componentValues.size() == 1 ? componentValues.front() : componentValues[componentIndex];
        const prm::Default component = readDefault(YAML::Node(componentText), valueType);

        switch (valueType)
        {
        case prm::ValueType::Float:
            value.floatValues.push_back(component.getFloat());
            break;
        case prm::ValueType::Int:
            value.intValues.push_back(component.getInt());
            break;
        case prm::ValueType::String:
            value.stringValues.push_back(component.getString());
            break;
        }
    }
    return value;
}

// Rejects every top level key the implementation kind does not take.
void validateKeysForKind(
    const YAML::Node& document,
    const NodeImplementation& implementation,
    const std::string& nodeName
)
{
    const std::set<std::string> cppKeys = {
        "version",
        "name",
        "namespace",
        "label",
        "tags",
        "implementation",
        "icon",
        "docs",
        "childScopeType",
        "inputs",
        "outputs",
        "parameters",
    };
    const std::set<std::string> aliasKeys = {
        "version",
        "name",
        "namespace",
        "label",
        "tags",
        "implementation",
        "parameterValues",
    };

    const bool isAlias = std::holds_alternative<AliasImplementation>(implementation);
    const std::set<std::string>& allowedKeys = isAlias ? aliasKeys : cppKeys;

    for (const auto& entry : document)
    {
        const std::string key = entry.first.as<std::string>();
        if (!allowedKeys.contains(key))
            throw std::runtime_error("node " + nodeName + " cannot declare " + key);
    }
}

std::vector<InputPort> readInputPorts(const YAML::Node& inputs, const std::string& nodeName)
{
    std::vector<InputPort> parsed;
    for (const YAML::Node& input : inputs)
    {
        const bool followsAMultiInputPort = !parsed.empty() && parsed.back().multiInput;
        if (followsAMultiInputPort)
            throw std::runtime_error(
                "node " + nodeName + " declares input " + parsed.back().label +
                " as a multi input port but it is not the last port"
            );

        InputPort declared;
        declared.label = requireString(input, "label", "node " + nodeName + " input");
        declared.multiInput = input["multiInput"] && input["multiInput"].as<bool>();
        declared.optional = input["optional"] && input["optional"].as<bool>();
        parsed.push_back(std::move(declared));
    }

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

    manifest.implementation_ =
        readImplementation(document["implementation"], nodeType.internalName);
    validateKeysForKind(document, manifest.implementation_, nodeType.internalName);

    nodeType.templates = readParameters(document["parameters"]);
    validateStyles(nodeType.templates);
    nodeType.tags = readTags(document["tags"]);
    nodeType.iconPath = readString(document, "icon");
    nodeType.docsPath = readString(document, "docs");
    nodeType.childScopeType = readString(document, "childScopeType");

    nodeType.inputPorts = readInputPorts(document["inputs"], nodeType.internalName);
    if (document["outputs"]) nodeType.maxOutputs = document["outputs"].as<unsigned int>();

    manifest.parameterValues_ = readParameterValues(document["parameterValues"]);

    return manifest;
}

NodeAlias NodeManifest::getNodeAlias(const NodeType& aliasedType) const
{
    NodeAlias alias;
    alias.internalName = nodeType_.internalName;
    alias.typeNamespace = nodeType_.typeNamespace;
    alias.displayName = nodeType_.displayName;
    alias.tags = nodeType_.tags;
    alias.aliasedType = std::get<AliasImplementation>(implementation_).aliasedType;

    for (const auto& [parameterName, componentValues] : parameterValues_)
    {
        const prm::Template* parameter = getParameter(aliasedType.templates, parameterName);
        if (!parameter)
            throw std::runtime_error(
                "alias " + alias.getFullName() + " sets " + parameterName + ", which " +
                aliasedType.getFullName() + " does not have"
            );

        if (parameter->isMultiParm())
            throw std::runtime_error(
                "alias " + alias.getFullName() + " sets " + parameterName +
                ", which holds instances rather than a value"
            );

        const unsigned int componentCount = parameter->getSize();
        const bool fitsParameter =
            componentValues.size() == 1 || componentValues.size() == componentCount;
        if (!fitsParameter)
            throw std::runtime_error(
                "alias " + alias.getFullName() + " gives " + parameterName + " " +
                std::to_string(componentValues.size()) + " values but it has " +
                std::to_string(componentCount) + " components"
            );

        alias.parameterValues.push_back(readParameterValue(*parameter, componentValues));
    }

    return alias;
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
