#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariant>

namespace enzo::ui {

/// @brief View-model backing the theme page of the settings window.
///
/// Lists the themes to choose between, the palette of the chosen one, and the
/// edits made to it.
///
/// @note An edit repaints the interface straight away and is written to the
/// theme file only on apply.
class ThemeViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY currentThemeChanged)
    Q_PROPERTY(bool editable READ editable NOTIFY currentThemeChanged)
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)
    Q_PROPERTY(QStringList modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool hasPendingChanges READ hasPendingChanges NOTIFY modifiedChanged)
    Q_PROPERTY(QStringList fontFamilies READ fontFamilies CONSTANT)

  public:
    explicit ThemeViewModel(QObject* parent = nullptr);

    /// @brief Returns every theme as `{ name, editable, swatch }`, the shipped one first.
    QVariantList themes() const;

    /// @brief Returns the name of the theme the interface is drawn in.
    QString currentTheme() const;

    /// @brief Returns whether the current theme's palette can be edited.
    bool editable() const;

    /// @brief Returns the palette as `{ title, entries }` sections.
    ///
    /// An entry is `{ name, label, kind }`, where kind is `color`, `number` or `font`.
    QVariantList sections() const;

    /// @brief Returns the names of the values edited since the last apply.
    QStringList modified() const;

    /// @brief Returns whether the current theme has edits that are not written yet.
    bool hasPendingChanges() const;

    /// @brief Returns the font families a type entry can be set to.
    QStringList fontFamilies() const;

    /// @brief Sets the font families a type entry can be set to.
    void setFontFamilies(const QStringList& families);

    /// @brief Draws the interface in another theme, dropping any pending edits.
    Q_INVOKABLE void selectTheme(const QString& name);

    /// @brief Sets one palette value and repaints the interface in it.
    Q_INVOKABLE void setValue(const QString& name, const QVariant& value);

    /// @brief Writes the pending edits to the theme file.
    Q_INVOKABLE void apply();

    /// @brief Drops the pending edits and repaints the interface in the written theme.
    Q_INVOKABLE void revert();

    /// @brief Copies the current theme to a new editable one and selects it.
    /// @return The name of the copy.
    Q_INVOKABLE QString duplicateTheme();

    /// @brief Renames the current theme, keeping its edits.
    Q_INVOKABLE void renameTheme(const QString& name);

    /// @brief Deletes the current theme and falls back to the shipped one.
    Q_INVOKABLE void deleteTheme();

    /// @brief Opens the theme folder in the desktop's file browser.
    Q_INVOKABLE void openThemeFolder();

  Q_SIGNALS:
    void themesChanged();
    void currentThemeChanged();
    void sectionsChanged();
    void modifiedChanged();

  private:
    /// @brief Draws the interface in a theme, dropping any pending edits.
    void switchToTheme(const QString& name);

    /// @brief Repaints the interface in the current theme with the pending edits on top.
    void preview();

    /// @brief Returns the YAML of the current theme with the pending edits on top.
    QString currentYaml() const;

    /// @brief The families the program ships, which a type entry chooses between.
    QStringList fontFamilies_;

    /// @brief The YAML of the current theme as its file holds it.
    QString writtenYaml_;

    /// @brief The values edited since the last apply, keyed by variable name.
    QHash<QString, QVariant> pending_;
};

} // namespace enzo::ui
