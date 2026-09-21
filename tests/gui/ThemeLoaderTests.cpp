#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QColor>
#include <QDir>
#include <QVariant>

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "Gui/Style/ThemeLoader.h"

using enzo::ui::ThemeLoader;

namespace {

const std::string kDefaultThemePath = ENZO_DEV_STATIC_DIR "/theme/default.yml";

/// @brief Returns the path of every theme that ships alongside the default.
std::vector<std::string> shippedThemePaths()
{
    const QDir themes(ENZO_DEV_STATIC_DIR "/theme");

    std::vector<std::string> paths;
    for (const QString& file : themes.entryList({"*.yml"}, QDir::Files, QDir::Name))
    {
        if (file == "default.yml") continue;
        paths.push_back(themes.filePath(file).toStdString());
    }
    return paths;
}

const QString kDefaultTheme = R"(
variables:
  accent: "#8b5cf6"
  fieldSurface: "#0d0d11"
components:
  node:
    portColor: "#4a4a54"
    width: 80
  nodeLink:
    activeColor: $accent
  parameter:
    backgroundColor: $fieldSurface
    borderRadius: 10
)";

} // namespace

TEST_CASE("Variables and slots resolve into tokens")
{
    const auto tokens = ThemeLoader::loadFromString(kDefaultTheme);

    REQUIRE(tokens.value("var.accent").value<QColor>() == QColor("#8b5cf6"));
    REQUIRE(tokens.value("node.portColor").value<QColor>() == QColor("#4a4a54"));
    REQUIRE(tokens.value("node.width").toInt() == 80);
    REQUIRE(tokens.value("parameter.backgroundColor").value<QColor>() == QColor("#0d0d11"));
}

TEST_CASE("An opacity modifier fades a colour")
{
    const QString theme = R"(
variables:
  accent: "#8b5cf6"
components:
  nodeLink:
    activeColor: $accent @ 0.5
)";
    const auto tokens = ThemeLoader::loadFromString(theme);
    const QColor faded = tokens.value("nodeLink.activeColor").value<QColor>();

    REQUIRE(faded.red() == 139);
    REQUIRE(faded.alphaF() == Catch::Approx(0.5).margin(0.01));
}

TEST_CASE("A variable cycle is an error")
{
    const QString theme = R"(
variables:
  a: $b
  b: $a
components:
  node:
    portColor: "#000000"
)";
    REQUIRE_THROWS(ThemeLoader::loadFromString(theme));
}

TEST_CASE("An undefined variable is an error")
{
    const QString theme = R"(
components:
  node:
    portColor: $missing
)";
    REQUIRE_THROWS(ThemeLoader::loadFromString(theme));
}

TEST_CASE("A theme without components is an error")
{
    REQUIRE_THROWS(ThemeLoader::loadFromString("variables:\n  accent: \"#8b5cf6\"\n"));
}

TEST_CASE("A user theme overrides a slot")
{
    const QString user = R"(
components:
  node:
    portColor: "#111111"
)";
    const auto tokens = ThemeLoader::loadFromString(kDefaultTheme, user);

    REQUIRE(tokens.value("node.portColor").value<QColor>() == QColor("#111111"));
}

TEST_CASE("A changed variable reaches the slots that use it")
{
    const QString user = R"(
variables:
  accent: "#ff0000"
)";
    const auto tokens = ThemeLoader::loadFromString(kDefaultTheme, user);

    REQUIRE(tokens.value("var.accent").value<QColor>() == QColor("#ff0000"));
    REQUIRE(tokens.value("nodeLink.activeColor").value<QColor>() == QColor("#ff0000"));
}

TEST_CASE("A user override of the wrong type keeps the default")
{
    const QString user = R"(
components:
  parameter:
    borderRadius: "#ffffff"
)";
    const auto tokens = ThemeLoader::loadFromString(kDefaultTheme, user);

    REQUIRE(tokens.value("parameter.borderRadius").toInt() == 10);
}

TEST_CASE("An unknown user slot is ignored")
{
    const QString user = R"(
components:
  node:
    nonsense: 5
)";
    const auto tokens = ThemeLoader::loadFromString(kDefaultTheme, user);

    REQUIRE_FALSE(tokens.contains("node.nonsense"));
}

TEST_CASE("An unparseable user theme falls back to the default")
{
    const auto tokens = ThemeLoader::loadFromString(kDefaultTheme, "node: [1, 2");

    REQUIRE(tokens.value("node.portColor").value<QColor>() == QColor("#4a4a54"));
}

TEST_CASE("The default theme resolves")
{
    const auto tokens = ThemeLoader::loadFromFile(QString::fromStdString(kDefaultThemePath));

    REQUIRE(tokens.value("var.accent").value<QColor>() == QColor("#8b5cf6"));
    REQUIRE(tokens.value("node.bodyColor") == tokens.value("var.selectedFill"));
    REQUIRE(tokens.value("viewport.backgroundColor") == tokens.value("var.surfaceHeader"));
}

TEST_CASE("Every colour the default theme gives a component comes from a variable")
{
    const YAML::Node theme = YAML::LoadFile(kDefaultThemePath);

    // The display flag holds its own colour rather than reading a variable.
    const std::string exemptSlot = "displayFlagColor";

    for (const auto& component : theme["components"])
        for (const auto& slot : component.second)
        {
            const std::string slotName = slot.first.Scalar();
            if (slotName == exemptSlot) continue;

            const std::string value = slot.second.Scalar();
            INFO(component.first.Scalar() << '.' << slotName << " is " << value);
            REQUIRE(value.find('#') == std::string::npos);
        }
}

TEST_CASE("Every entry the palette lists names a variable")
{
    const YAML::Node theme = YAML::LoadFile(kDefaultThemePath);

    for (const auto& section : theme["palette"])
        for (const auto& entry : section.second)
        {
            const std::string name = entry.first.Scalar();
            INFO(section.first.Scalar() << " lists " << name);
            REQUIRE(theme["variables"][name]);
        }
}

TEST_CASE("Every shipped theme resolves over the default")
{
    const auto defaultTokens = ThemeLoader::loadFromFile(QString::fromStdString(kDefaultThemePath));

    for (const std::string& path : shippedThemePaths())
    {
        INFO(path);
        const auto tokens = ThemeLoader::loadFromFile(
            QString::fromStdString(kDefaultThemePath),
            QString::fromStdString(path)
        );

        REQUIRE(tokens.value("var.accent") != defaultTokens.value("var.accent"));
        REQUIRE(tokens.value("node.bodyColor") == tokens.value("var.selectedFill"));
    }
}

TEST_CASE("Every value a shipped theme lays over the default is one the default defines")
{
    const YAML::Node defaultTheme = YAML::LoadFile(kDefaultThemePath);

    for (const std::string& path : shippedThemePaths())
    {
        const YAML::Node theme = YAML::LoadFile(path);
        INFO(path);

        for (const auto& variable : theme["variables"])
        {
            const std::string name = variable.first.Scalar();
            INFO("sets " << name);
            REQUIRE(defaultTheme["variables"][name]);
        }

        for (const auto& component : theme["components"])
            for (const auto& slot : component.second)
            {
                const std::string name = component.first.Scalar();
                INFO("sets " << name << '.' << slot.first.Scalar());
                REQUIRE(defaultTheme["components"][name][slot.first.Scalar()]);
            }
    }
}
