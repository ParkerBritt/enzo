#pragma once

#include <QHash>
#include <QString>
#include <QVariant>

namespace enzo::ui {

/// @brief Resolves a YAML theme into a flat table of design tokens.
///
/// Tokens are keyed `group.slot`, with `var.<name>` for the shared palette. A value
/// is a colour, a number, a font name, or a list of those.
///
/// The default theme must define every token, so any fault in it throws. The optional
/// user theme only overrides, and a faulty entry there falls back to the default.
class ThemeLoader
{
  public:
    /// @brief Returns the resolved tokens for the default theme with the user theme on top.
    /// @note Throws std::runtime_error when the default file is missing or malformed.
    static QHash<QString, QVariant>
    loadFromFile(const QString& defaultPath, const QString& userPath = {});

    /// @brief Returns the resolved tokens from in-memory YAML.
    static QHash<QString, QVariant>
    loadFromString(const QString& defaultYaml, const QString& userYaml = {});
};

} // namespace enzo::ui
