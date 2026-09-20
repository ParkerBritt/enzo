#pragma once

#include <QString>
#include <QStringList>

namespace enzo::ui {

/// @brief Returns the folder holding the user's theme files.
/// @note Creates the folder when it is missing.
QString themesDir();

/// @brief Returns the name of the theme that ships with the program.
QString shippedThemeName();

/// @brief Returns the YAML of the theme that ships with the program.
QString shippedThemeYaml();

/// @brief Returns the names of the user's themes, alphabetically.
QStringList userThemeNames();

/// @brief Returns the name of the theme the interface is drawn in.
QString activeThemeName();

/// @brief Records which theme the interface is drawn in.
void setActiveThemeName(const QString& name);

/// @brief Returns a user theme's YAML, empty for the shipped theme or a missing file.
QString readThemeYaml(const QString& name);

/// @brief Writes a user theme's YAML, replacing any file already under that name.
void writeThemeYaml(const QString& name, const QString& yaml);

/// @brief Deletes a user theme's file.
void deleteThemeFile(const QString& name);

} // namespace enzo::ui
