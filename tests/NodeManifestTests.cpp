#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeManifest.h"
#include "Engine/Parameter/Style.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

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
    REQUIRE(manifest.getImplementation().kind == "cpp");
    REQUIRE(manifest.getImplementation().library == "enzoOps");
}

TEST_CASE("A missing label falls back to the node name")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);

    REQUIRE(manifest.getNodeType().getLabel() == "circle");
}

TEST_CASE("A missing constructor falls back to the node name")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);

    REQUIRE(manifest.getImplementation().constructor == "circle");
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

    REQUIRE(manifest.getImplementation().constructor == "circle");
}

TEST_CASE("Input and output counts are read from the manifest")
{
    const std::string yaml = R"(
version: 1
name: sweep
namespace: enzo
inputs:
  min: 1
  max: 2
outputs: 3
implementation:
  kind: cpp
  library: enzoOps
)";
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(yaml);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.minInputs == 1);
    REQUIRE(nodeType.maxInputs == 2);
    REQUIRE(nodeType.maxOutputs == 3);
}

TEST_CASE("Missing counts leave a node with one input and one output")
{
    const nt::NodeManifest manifest = nt::NodeManifest::loadFromString(kMinimalManifest);
    const nt::NodeType& nodeType = manifest.getNodeType();

    REQUIRE(nodeType.minInputs == 0);
    REQUIRE(nodeType.maxInputs == 1);
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
    type: xyz
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
    type: xyz
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
    tooltip: How much of the circle to keep.
    documentation: The arc controls.
    disableCondition: applyScale == 0
    hideCondition: profileShape != 1
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);

    REQUIRE(parameter.getDirection() == prm::Direction::VERTICAL);
    REQUIRE(parameter.isBackgroundEnabled() == false);
    REQUIRE(parameter.isLabelHidden() == true);
    REQUIRE(parameter.getTooltip() == "How much of the circle to keep.");
    REQUIRE(parameter.getDocumentation() == "The arc controls.");
    REQUIRE(parameter.getDisableCondition() == "applyScale == 0");
    REQUIRE(parameter.getHideCondition() == "profileShape != 1");
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
    style:
      kind: boolIconSlash
      icon: squares-subtract
      scale: 0.5
)");
    const prm::Template& parameter = manifest.getNodeType().templates.at(0);
    const auto style =
        std::any_cast<std::shared_ptr<prm::style::BoolIconSlash>>(parameter.getStyle());

    REQUIRE(style->icon() == "squares-subtract");
    REQUIRE(style->scale() == Catch::Approx(0.5));
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
    style:
      kind: boolWobble
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
    REQUIRE(nodeType.minInputs == 1);
    REQUIRE(nodeType.maxInputs == 2);
    REQUIRE(manifest.getImplementation().library == "enzoOps");

    // The parameters, in the order the node's interface is built from.
    REQUIRE(nodeType.templates.size() == 9);
    REQUIRE(nodeType.templates.at(0).getName() == "profileShape");
    REQUIRE(nodeType.templates.at(0).getDefault().getString() == "round");
    REQUIRE(nodeType.templates.at(2).getHideCondition() == "profileShape != 1");

    const prm::Template& capGroup =
        nodeType.templates.at(5).getChild("endCapGroup").getChild("endCapGroupName");
    REQUIRE(capGroup.getDefault().getString() == "sweepCap");
    REQUIRE(capGroup.isBackgroundEnabled() == false);

    const prm::Template& scaleRamp = nodeType.templates.at(8);
    REQUIRE(scaleRamp.getInstanceDefault("value", 0)->getFloat() == Catch::Approx(1));
}
