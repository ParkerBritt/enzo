#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeManifest.h"
#include "Engine/Parameter/Styles.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <map>
#include <string>
#include <variant>
#include <vector>

using namespace enzo;

namespace {

// The smallest manifest that parses, used as the base for the focused cases so
// each one shows only the keys it is about.
const std::string kMinimalManifest = R"(
version: 1
name: circle
namespace: enzo
implementation:
  kind: cpp
  library: enzoOps
)";

// The smallest alias manifest that parses.
const std::string kAliasManifest = R"(
version: 1
name: mountain
namespace: enzo
implementation:
  kind: alias
  type: enzo::attributeNoise
)";

// A node for kAliasManifest to stand in for, with one parameter of each kind an
// alias can set and a multiparm it cannot.
const std::string kAliasedManifest = R"(
version: 1
name: attributeNoise
namespace: enzo
label: Attribute Noise
tags: [noise]
inputs:
  - label: Geometry
implementation:
  kind: cpp
  library: enzoOps
parameters:
  - name: name
    type: string
    default: noise
  - name: type
    type: dropdown
    options:
      - {value: float}
      - {value: vector}
  - name: amplitude
    type: float
    default: 1
  - name: offset
    type: float
    size: 3
  - name: alongVectorRow
    type: group
    parameters:
      - name: alongVector
        type: bool
  - name: falloff
    type: ramp
)";

// Returns the minimal manifest with a parameter block bolted on. The manifest
// owns its templates, so callers keep it alive for as long as they read one.
nt::NodeManifest manifestWithParameters(const std::string& parameters)
{
    return nt::NodeManifest::loadFromString(kMinimalManifest + "parameters:\n" + parameters);
}

} // namespace

TEST_CASE("A minimal manifest gives the node its name and implementation")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);

    REQUIRE(manifest.getNodeType().getName() == "circle");
    REQUIRE(manifest.getNodeType().getFullName() == "enzo::circle");
    const auto& implementation = std::get<nt::CppImplementation>(manifest.getImplementation());
    REQUIRE(implementation.library == "enzoOps");
}

TEST_CASE("A missing label falls back to the node name")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);

    REQUIRE(manifest.getNodeType().getLabel() == "circle");
}

TEST_CASE("A missing constructor falls back to the node name")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);

    REQUIRE(std::get<nt::CppImplementation>(manifest.getImplementation()).constructor == "circle");
}

TEST_CASE("A named constructor is kept as written")
{
    const std::string yaml = R"(
version: 1
name: hexagon
namespace: enzo
implementation:
  kind: cpp
  library: enzoOps
  constructor: circle
)";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(yaml);

    REQUIRE(std::get<nt::CppImplementation>(manifest.getImplementation()).constructor == "circle");
}

TEST_CASE("An alias names the node type it stands in for")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kAliasManifest);

    REQUIRE(manifest.getNodeType().getFullName() == "enzo::mountain");
    const auto& implementation = std::get<nt::AliasImplementation>(manifest.getImplementation());
    REQUIRE(implementation.aliasedType == "enzo::attributeNoise");
}

TEST_CASE("An alias reads its parameter values as written")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kAliasManifest + R"(
parameterValues:
  name: P
  offset: [0, 1, 0]
)");

    const std::map<std::string, std::vector<std::string>> expected = {
        {"name", {"P"}},
        {"offset", {"0", "1", "0"}},
    };
    REQUIRE(manifest.getParameterValues() == expected);
}

TEST_CASE("An alias with no parameter values sets none")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kAliasManifest);

    REQUIRE(manifest.getParameterValues().empty());
}

TEST_CASE("Inputs and the output count are read from the manifest")
{
    const std::string yaml = R"(
version: 1
name: sweep
namespace: enzo
inputs:
  - label: Backbone
  - label: Profile
outputs: 3
implementation:
  kind: cpp
  library: enzoOps
)";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(yaml);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.inputPorts.size() == 2);
    REQUIRE(nodeType.inputPorts.at(0).label == "Backbone");
    REQUIRE(nodeType.inputPorts.at(1).label == "Profile");
    REQUIRE(nodeType.maxOutputs == 3);
}

TEST_CASE("An input is required unless the manifest marks it optional")
{
    const std::string yaml = R"(
version: 1
name: sweep
namespace: enzo
inputs:
  - label: Backbone
  - label: Profile
    optional: true
implementation:
  kind: cpp
  library: enzoOps
)";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(yaml);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(!nodeType.inputPorts.at(0).optional);
    REQUIRE(nodeType.inputPorts.at(1).optional);
}

TEST_CASE("An input with no label is rejected")
{
    const std::string yaml = R"(
version: 1
name: blur
namespace: enzo
inputs:
  - multiInput: true
implementation:
  kind: cpp
  library: enzoOps
)";
    REQUIRE_THROWS(nt::NodeManifest::loadFromString(yaml));
}

TEST_CASE("A multi input port can be declared last")
{
    const std::string yaml = R"(
version: 1
name: merge
namespace: enzo
inputs:
  - label: Geometry
    multiInput: true
implementation:
  kind: cpp
  library: enzoOps
)";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(yaml);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.inputPorts.size() == 1);
    REQUIRE(nodeType.inputPorts.at(0).label == "Geometry");
    REQUIRE(nodeType.inputPorts.at(0).multiInput);
}

TEST_CASE("A multi input port before another port is rejected")
{
    const std::string yaml = R"(
version: 1
name: broken
namespace: enzo
inputs:
  - label: Cutters
    multiInput: true
  - label: Geometry
implementation:
  kind: cpp
  library: enzoOps
)";
    REQUIRE_THROWS(nt::NodeManifest::loadFromString(yaml));
}

TEST_CASE("A manifest naming no inputs declares a node that takes none")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.inputPorts.empty());
    REQUIRE(nodeType.maxOutputs == 1);
}

TEST_CASE("Namespace, tags, icon and docs are read from the manifest")
{
    const std::string yaml = R"(
version: 1
name: circle
namespace: enzo
tags: [primitive, curve]
icon: icon.svg
docs: docs.md
implementation:
  kind: cpp
  library: enzoOps
)";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(yaml);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.typeNamespace == "enzo");
    REQUIRE(nodeType.tags == std::vector<std::string>{"primitive", "curve"});
    REQUIRE(nodeType.iconPath == "icon.svg");
    REQUIRE(nodeType.docsPath == "docs.md");
}

TEST_CASE("A node declares the scope it holds inside it")
{
    const nt::NodeManifest manifest =
        nt::NodeManifest::loadFromString(kMinimalManifest + "childScopeType: geometry\n");

    REQUIRE(manifest.getNodeType().hasChildScope());
    REQUIRE(manifest.getNodeType().childScopeType == "geometry");
}

TEST_CASE("A node holds no scope unless its manifest names one")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);

    REQUIRE_FALSE(manifest.getNodeType().hasChildScope());
    REQUIRE(manifest.getNodeType().childScopeType.empty());
}

TEST_CASE("A parameter carries its label, default and range")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: divisions
    label: Divisions
    type: int
    default: 10
    range: {min: 1, max: 100, minLocked: true}
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getName() == "divisions");
    REQUIRE(parameter.getLabel() == "Divisions");
    REQUIRE(parameter.getType() == prm::Type::INT);
    REQUIRE(parameter.getDefault().getInt() == 10);
    REQUIRE(parameter.getRange().getMin() == 1);
    REQUIRE(parameter.getRange().getMax() == 100);
    REQUIRE(parameter.getRange().getMinFlag() == prm::RangeFlag::LOCKED);
    REQUIRE(parameter.getRange().getMaxFlag() == prm::RangeFlag::UNLOCKED);
}

TEST_CASE("A parameter with no label falls back to its name")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: divisions
    type: int
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getLabel() == "divisions");
}

TEST_CASE("A bool default is written either as a number or as true and false")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: spelled
    type: bool
    default: true
  - name: numbered
    type: bool
    default: 1
)");
    const std::vector<prm::Template>& parameters = manifest.getNodeType().templates;

    REQUIRE(parameters.at(0).getDefault().getInt() == 1);
    REQUIRE(parameters.at(1).getDefault().getInt() == 1);
}

TEST_CASE("One default covers every component of a vector parameter")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: center
    type: float
    size: 3
    default: 2.5
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getSize() == 3);
    REQUIRE(parameter.getNumDefaults() == 3);
    REQUIRE(parameter.getDefault(2).getFloat() == Catch::Approx(2.5));
}

TEST_CASE("A list of defaults gives each component its own value")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: scale
    type: float
    size: 3
    default: [1, 2, 3]
    range: {min: 0, max: 5}
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getDefault(0).getFloat() == Catch::Approx(1));
    REQUIRE(parameter.getDefault(2).getFloat() == Catch::Approx(3));
    // The one range spreads across the components the list left uncovered.
    REQUIRE(parameter.getRange(2).getMax() == 5);
}

TEST_CASE("A dropdown keeps its options in the order they are written")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: orientation
    type: dropdown
    default: zx
    options:
      - {value: xy, label: XY Plane}
      - {value: yz, label: YZ Plane}
      - {value: zx, label: ZX Plane}
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getNumOptions() == 3);
    REQUIRE(parameter.getOptions().at(0).getToken() == "xy");
    REQUIRE(parameter.getOptions().at(0).getLabel() == "XY Plane");
    REQUIRE(parameter.getDefault().getString() == "zx");
}

TEST_CASE("An option with no label falls back to its value")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: orientation
    type: dropdown
    options:
      - {value: xy}
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getOptions().at(0).getLabel() == "xy");
}

TEST_CASE("Conditions, tooltips and layout flags reach the template")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: arcGroup
    label: Arc
    type: group
    direction: vertical
    background: false
    labelHidden: true
    labelInline: true
    tooltip: How much of the circle to keep.
    documentation: The arc controls.
    disableCondition: applyScale == 0
    hideCondition: profileShape != 1
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getDirection() == prm::Direction::VERTICAL);
    REQUIRE(parameter.isBackgroundEnabled() == false);
    REQUIRE(parameter.isLabelHidden() == true);
    REQUIRE(parameter.isLabelInline() == true);
    REQUIRE(parameter.getTooltip() == "How much of the circle to keep.");
    REQUIRE(parameter.getDocumentation() == "The arc controls.");
    REQUIRE(parameter.getDisableCondition() == "applyScale == 0");
    REQUIRE(parameter.getHideCondition() == "profileShape != 1");
}

TEST_CASE("A divider carries its label and icon")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: shapeDivider
    label: Shape
    type: divider
    icon: box
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getType() == prm::Type::DIVIDER);
    REQUIRE(parameter.getLabel() == "Shape");
    REQUIRE(parameter.getIcon() == "box");
    REQUIRE(parameter.isBackgroundEnabled() == false);
}

TEST_CASE("Groups nest through their parameters key")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: outerGroup
    type: group
    parameters:
      - name: innerGroup
        type: group
        parameters:
          - name: capName
            type: string
            default: sweepCap
)");
    const prm::Template& outerGroup = manifest.getNodeType().templates.at(0);
    const prm::Template& capName = outerGroup.getChild("innerGroup").getChild("capName");

    REQUIRE(outerGroup.isContainer());
    REQUIRE(capName.getDefault().getString() == "sweepCap");
}

TEST_CASE("A style is attached by its name")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: capGroupEnabled
    type: bool
    style: boolIcon
    styleOptions:
      icon: squares-subtract
      scale: 0.5
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);
    const auto style =
        std::any_cast<std::shared_ptr<prm::style::BoolIcon>>(parameter.getStyle());

    REQUIRE(style->icon() == "squares-subtract");
    REQUIRE(style->scale() == Catch::Approx(0.5));
}

TEST_CASE("An xyz style marks a float vector as axis components")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: center
    type: float
    size: 3
    style: xyz
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getType() == prm::Type::FLOAT);
    REQUIRE(parameter.getSize() == 3);
    REQUIRE(std::any_cast<std::shared_ptr<prm::style::Xyz>>(parameter.getStyle()) != nullptr);
}

TEST_CASE("A range circle style marks a float pair as a dial")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: arc_angles
    type: float
    size: 2
    default: [0, 360]
    style: rangeCircle
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getSize() == 2);
    REQUIRE(
        std::any_cast<std::shared_ptr<prm::style::RangeCircle>>(parameter.getStyle()) != nullptr
    );
}

TEST_CASE("A range circle keeps its bounds in order when asked to")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: arc_angles
    type: float
    size: 2
    style: rangeCircle
    styleOptions:
      ordered: true
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);
    const auto style = std::any_cast<std::shared_ptr<prm::style::RangeCircle>>(parameter.getStyle());

    REQUIRE(style->ordered());
}

TEST_CASE("An attribute name style carries the attributes it offers")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: name
    type: string
    style: attribute
    styleOptions:
      owners: point vertex
      attributeTypes: float vector
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);
    const auto style =
        std::any_cast<std::shared_ptr<prm::style::Attribute>>(parameter.getStyle());

    REQUIRE(style->owners() == "point vertex");
    REQUIRE(style->attributeTypes() == "float vector");
}

TEST_CASE("A style option reads its value from another parameter")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: name
    type: string
    style: attribute
    styleOptions:
      owners: prm(attachTo)
  - name: attachTo
    type: dropdown
    options:
      - {value: point, label: Point}
      - {value: vertex, label: Vertex}
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);
    const auto style = std::any_cast<std::shared_ptr<prm::style::Attribute>>(parameter.getStyle());

    REQUIRE(style->owners() == "prm(attachTo)");
}

TEST_CASE("Instance defaults set the starting values of a multiparm")
{
    const nt::NodeManifest manifest = manifestWithParameters(R"(
  - name: scaleRamp
    type: ramp
    default: 2
    instanceDefaults:
      value: [0.25, 0.75]
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.isMultiParm());
    REQUIRE(parameter.getDefault().getInt() == 2);
    REQUIRE(parameter.getInstanceDefault("value", 0)->getFloat() == Catch::Approx(0.25));
    REQUIRE(parameter.getInstanceDefault("value", 1)->getFloat() == Catch::Approx(0.75));
}

// Error reporting

TEST_CASE("A manifest of an unsupported schema version is rejected")
{
    const std::string yaml = R"(
version: 2
name: circle
implementation:
  kind: cpp
  library: enzoOps
)";
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromString(yaml), std::runtime_error);
}

TEST_CASE("A manifest with no name is rejected")
{
    const std::string yaml = R"(
version: 1
implementation:
  kind: cpp
  library: enzoOps
)";
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromString(yaml), std::runtime_error);
}

TEST_CASE("A manifest with no namespace is rejected")
{
    const std::string yaml = R"(
version: 1
name: circle
implementation:
  kind: cpp
  library: enzoOps
)";
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromString(yaml), std::runtime_error);
}

TEST_CASE("A manifest with no implementation is rejected")
{
    const std::string yaml = R"(
version: 1
name: circle
namespace: enzo
)";
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromString(yaml), std::runtime_error);
}

TEST_CASE("An implementation kind that is not built yet is rejected")
{
    const std::string yaml = R"(
version: 1
name: torus
namespace: enzo
implementation:
  kind: compound
  network: network.enzo
)";
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromString(yaml), std::runtime_error);
}

TEST_CASE("An alias with no type is rejected")
{
    const std::string yaml = R"(
version: 1
name: mountain
namespace: enzo
implementation:
  kind: alias
)";
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromString(yaml), std::runtime_error);
}

TEST_CASE("An alias declaring anything the node it stands in for provides is rejected")
{
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(
            kAliasManifest + "parameters:\n  - {name: seed, type: int}\n"
        ),
        std::runtime_error
    );
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kAliasManifest + "inputs:\n  - label: Geometry\n"),
        std::runtime_error
    );
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kAliasManifest + "outputs: 2\n"),
        std::runtime_error
    );
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kAliasManifest + "icon: icon.svg\n"),
        std::runtime_error
    );
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kAliasManifest + "docs: docs.md\n"),
        std::runtime_error
    );
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kAliasManifest + "childScopeType: geometry\n"),
        std::runtime_error
    );
}

TEST_CASE("A top level key the manifest does not know is rejected")
{
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kMinimalManifest + "paramters: []\n"),
        std::runtime_error
    );
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kAliasManifest + "paramterValues: {}\n"),
        std::runtime_error
    );
}

TEST_CASE("Parameter values on a node that is not an alias are rejected")
{
    REQUIRE_THROWS_AS(
        nt::NodeManifest::loadFromString(kMinimalManifest + "parameterValues:\n  seed: 4\n"),
        std::runtime_error
    );
}

TEST_CASE("An alias carries its own name and the node type it stands in for")
{
    const nt::NodeManifest aliasedManifest = nt::NodeManifest::loadFromString(kAliasedManifest);
    const nt::NodeManifest aliasManifest = nt::NodeManifest::loadFromString(kAliasManifest + R"(
label: Mountain
tags: [terrain]
)");

    const nt::NodeAlias alias = aliasManifest.getNodeAlias(aliasedManifest.getNodeType());

    REQUIRE(alias.getFullName() == "enzo::mountain");
    REQUIRE(alias.getLabel() == "Mountain");
    REQUIRE(alias.tags == std::vector<std::string>{"terrain"});
    REQUIRE(alias.aliasedType == "enzo::attributeNoise");
}

TEST_CASE("An alias reads each parameter value as the type of the parameter it sets")
{
    const nt::NodeManifest aliasedManifest = nt::NodeManifest::loadFromString(kAliasedManifest);
    const nt::NodeManifest aliasManifest = nt::NodeManifest::loadFromString(kAliasManifest + R"(
parameterValues:
  name: P
  type: vector
  amplitude: 2.5
  offset: [0, 1, 0]
  alongVector: true
)");

    const nt::NodeAlias alias = aliasManifest.getNodeAlias(aliasedManifest.getNodeType());

    std::map<std::string, ParameterSerializable> valuesByName;
    for (const ParameterSerializable& value : alias.parameterValues)
        valuesByName[value.name] = value;

    // Flat parameters
    REQUIRE(valuesByName.at("name").stringValues == std::vector<String>{"P"});
    REQUIRE(valuesByName.at("type").stringValues == std::vector<String>{"vector"});
    REQUIRE(valuesByName.at("amplitude").floatValues == std::vector<floatT>{2.5});

    // One value per component
    REQUIRE(valuesByName.at("offset").floatValues == std::vector<floatT>{0, 1, 0});

    // A parameter nested in a group
    REQUIRE(valuesByName.at("alongVector").intValues == std::vector<intT>{1});
}

TEST_CASE("One alias value covers every component of a vector parameter")
{
    const nt::NodeManifest aliasedManifest = nt::NodeManifest::loadFromString(kAliasedManifest);
    const nt::NodeManifest aliasManifest =
        nt::NodeManifest::loadFromString(kAliasManifest + "parameterValues:\n  offset: 4\n");

    const nt::NodeAlias alias = aliasManifest.getNodeAlias(aliasedManifest.getNodeType());

    REQUIRE(alias.parameterValues.at(0).floatValues == std::vector<floatT>{4, 4, 4});
}

TEST_CASE("An alias value naming a parameter the node does not have is rejected")
{
    const nt::NodeManifest aliasedManifest = nt::NodeManifest::loadFromString(kAliasedManifest);
    const nt::NodeManifest aliasManifest =
        nt::NodeManifest::loadFromString(kAliasManifest + "parameterValues:\n  roughness: 1\n");

    REQUIRE_THROWS_AS(
        aliasManifest.getNodeAlias(aliasedManifest.getNodeType()),
        std::runtime_error
    );
}

TEST_CASE("An alias value with the wrong number of components is rejected")
{
    const nt::NodeManifest aliasedManifest = nt::NodeManifest::loadFromString(kAliasedManifest);
    const nt::NodeManifest aliasManifest =
        nt::NodeManifest::loadFromString(kAliasManifest + "parameterValues:\n  offset: [0, 1]\n");

    REQUIRE_THROWS_AS(
        aliasManifest.getNodeAlias(aliasedManifest.getNodeType()),
        std::runtime_error
    );
}

TEST_CASE("An alias value for a multiparm is rejected")
{
    const nt::NodeManifest aliasedManifest = nt::NodeManifest::loadFromString(kAliasedManifest);
    const nt::NodeManifest aliasManifest =
        nt::NodeManifest::loadFromString(kAliasManifest + "parameterValues:\n  falloff: 2\n");

    REQUIRE_THROWS_AS(
        aliasManifest.getNodeAlias(aliasedManifest.getNodeType()),
        std::runtime_error
    );
}

TEST_CASE("A parameter with an unknown type is rejected")
{
    const std::string parameters = R"(
  - name: divisions
    type: quaternion
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("A parameter with an unknown style is rejected")
{
    const std::string parameters = R"(
  - name: capGroupEnabled
    type: bool
    style: boolWobble
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("An attribute name style naming an owner that does not exist is rejected")
{
    const std::string parameters = R"(
  - name: name
    type: string
    style: attribute
    styleOptions:
      owners: corner
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("A style option reading a parameter the node does not have is rejected")
{
    const std::string parameters = R"(
  - name: name
    type: string
    style: attribute
    styleOptions:
      owners: prm(attachTo)
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("A style option reading a dropdown with an option it cannot use is rejected")
{
    const std::string parameters = R"(
  - name: name
    type: string
    style: attribute
    styleOptions:
      owners: prm(attachTo)
  - name: attachTo
    type: dropdown
    options:
      - {value: point, label: Point}
      - {value: corner, label: Corner}
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("A style option no option answers to is rejected")
{
    const std::string parameters = R"(
  - name: capGroupEnabled
    type: bool
    style: boolIcon
    styleOptions:
      wobble: 3
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("Style options without a style are rejected")
{
    const std::string parameters = R"(
  - name: capGroupEnabled
    type: bool
    styleOptions:
      icon: squares-subtract
)";
    REQUIRE_THROWS_AS(manifestWithParameters(parameters), std::runtime_error);
}

TEST_CASE("A missing manifest file is reported by its path")
{
    REQUIRE_THROWS_AS(nt::NodeManifest::loadFromFile("nowhere/node.yaml"), std::runtime_error);
}

// The shipped nodes

TEST_CASE("The sweep manifest parses into its node type")
{
    const std::filesystem::path path = nt::NodeLoader::getNodesDirectory() / "sweep/node.yaml";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromFile(path);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.getName() == "sweep");
    REQUIRE(nodeType.getLabel() == "Sweep");
    REQUIRE(nodeType.inputPorts.size() == 2);
    REQUIRE(nodeType.inputPorts.at(0).label == "Backbone");
    REQUIRE(nodeType.inputPorts.at(1).label == "Profile");
    REQUIRE(nodeType.inputPorts.at(1).optional);
    REQUIRE(std::get<nt::CppImplementation>(manifest.getImplementation()).library == "enzoOps");

    // The parameters, in the order the node's interface is built from.
    REQUIRE(nodeType.templates.size() == 12);
    REQUIRE(nodeType.templates.at(1).getName() == "profileShape");
    REQUIRE(nodeType.templates.at(1).getDefault().getString() == "round");
    REQUIRE(nodeType.templates.at(3).getHideCondition() == "profileShape != 1");

    const prm::Template& capGroup =
        nodeType.templates.at(7).getChild("endCapGroup").getChild("endCapGroupName");
    REQUIRE(capGroup.getDefault().getString() == "sweepCap");
    REQUIRE(capGroup.isBackgroundEnabled() == false);

    const prm::Template& scaleRamp = nodeType.templates.at(11);
    REQUIRE(scaleRamp.getInstanceDefault("value", 0)->getFloat() == Catch::Approx(1));
}
