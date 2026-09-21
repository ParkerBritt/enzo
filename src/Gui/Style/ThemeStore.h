#pragma once

#include <QString>
#include <QStringList>

namespace enzo::ui {

/// @brief Returns the folder holding the user's theme files.
/// @note Creates the folder when it is missing.
QString themesDir();

/// @brief Returns the YAML of the default theme, which defines every token.
QString baseThemeYaml();

/// @brief Returns the name of the default theme.
QString defaultThemeName();

/// @brief Returns the names of the themes that ship with the program, the default first.
///
/// @note The rest follow in alphabetical order.
QStringList builtinThemeNames();

/// @brief Returns whether a theme ships with the program.
bool isBuiltinTheme(const QString& name);

/// @brief Returns the names of the user's themes, alphabetically.
QStringList userThemeNames();

/// @brief Returns the name of the theme the interface is drawn in.
QString activeThemeName();

/// @brief Records which theme the interface is drawn in.
void setActiveThemeName(const QString& name);

/// @brief Returns the YAML a theme lays over the default, empty for the default itself.
QString readThemeYaml(const QString& name);

/// @brief Writes a user theme's YAML, replacing any file already under that name.
void writeThemeYaml(const QString& name, const QString& yaml);

/// @brief Deletes a user theme's file.
void deleteThemeFile(const QString& name);

} // namespace enzo::ui
