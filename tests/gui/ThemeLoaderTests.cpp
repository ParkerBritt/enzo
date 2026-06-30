#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QColor>
#include <QVariant>

#include "Gui/Style/ThemeLoader.h"

using enzo::ui::ThemeLoader;

namespace {

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

TEST_CASE("A list resolves to a list of values")
{
    const QString theme = R"(
components:
  spreadsheet:
    attributeOwnerColors:
      - "#4ea1ff"
      - "#f0a93b"
)";
    const auto tokens = ThemeLoader::loadFromString(theme);
    const QVariantList colours = tokens.value("spreadsheet.attributeOwnerColors").toList();

    REQUIRE(colours.size() == 2);
    REQUIRE(colours.at(0).value<QColor>() == QColor("#4ea1ff"));
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

TEST_CASE("The shipped default theme resolves")
{
    const auto tokens = ThemeLoader::loadFromFile(QStringLiteral(ENZO_DEV_STATIC_DIR "/theme/default.yml"));

    REQUIRE(tokens.value("var.accent").value<QColor>() == QColor("#8b5cf6"));
    REQUIRE(tokens.value("node.width").toInt() == 80);
    REQUIRE(tokens.value("spreadsheet.attributeOwnerColors").toList().size() == 5);
}
