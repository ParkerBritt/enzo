#include "Gui/Style/ThemeViewModel.h"
#include "Gui/Style/Theme.h"
#include "Gui/Style/ThemeLoader.h"
#include "Gui/Style/ThemeStore.h"

#include <QColor>
#include <QDesktopServices>
#include <QQmlPropertyMap>
#include <QUrl>
#include <QVariantMap>

#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace enzo::ui {

namespace {

constexpr auto kVariablesKey = "variables";
constexpr auto kPaletteKey = "palette";
constexpr auto kNameKey = "name";

/// @brief One palette section, holding its variables in the order they are listed.
struct PaletteSection
{
    QString title;
    std::vector<std::pair<QString, QString>> entries;
};

/// @brief Returns the palette the default theme lists, section by section.
const std::vector<PaletteSection>& paletteSections()
{
    static const std::vector<PaletteSection> sections = [] {
        std::vector<PaletteSection> parsed;
        const YAML::Node theme = YAML::Load(baseThemeYaml().toStdString());
        for (const auto& section : theme[kPaletteKey])
        {
            PaletteSection built;
            built.title = QString::fromStdString(section.first.Scalar());
            for (const auto& entry : section.second)
                built.entries.emplace_back(
                    QString::fromStdString(entry.first.Scalar()),
                    QString::fromStdString(entry.second.Scalar())
                );
            parsed.push_back(built);
        }
        return parsed;
    }();
    return sections;
}

/// @brief Returns the palette values the interface is currently drawn in.
QQmlPropertyMap* liveVariables()
{
    Theme* theme = Theme::instance();
    if (!theme) return nullptr;
    return qobject_cast<QQmlPropertyMap*>(theme->value(QStringLiteral("var")).value<QObject*>());
}

/// @brief Returns the kind of editor a value takes, `color`, `number` or `font`.
QString kindOf(const QVariant& value)
{
    if (value.typeId() == QMetaType::QColor) return QStringLiteral("color");
    if (value.typeId() == QMetaType::Double) return QStringLiteral("number");
    return QStringLiteral("font");
}

/// @brief Returns the YAML text a palette value is written as.
///
/// @note A colour keeps its hex form, so "#0a0a0d" reads back the way it was typed.
std::string yamlScalar(const QVariant& value)
{
    if (value.typeId() == QMetaType::QColor)
    {
        const QColor color = value.value<QColor>();
        const QColor::NameFormat format =
            color.alpha() == 255 ? QColor::HexRgb : QColor::HexArgb;
        return color.name(format).toStdString();
    }
    if (value.typeId() == QMetaType::Double)
        return QString::number(value.toDouble()).toStdString();
    return value.toString().toStdString();
}

/// @brief Returns the three colours that stand for a theme in the theme list.
QVariantList themeSwatch(const QString& themeYaml)
{
    QVariantList swatch;
    try
    {
        const QHash<QString, QVariant> tokens =
            ThemeLoader::loadFromString(baseThemeYaml(), themeYaml);
        for (const QString& token : {"var.background", "var.surface", "var.accent"})
            swatch.append(tokens.value(token));
    }
    catch (const std::exception&)
    {
    }
    return swatch;
}

/// @brief Writes theme YAML to a file of its own, titled with the given name.
void writeThemeAs(const QString& name, const QString& yaml)
{
    YAML::Node theme = YAML::Load(yaml.toStdString());
    theme[kNameKey] = name.toStdString();

    YAML::Emitter out;
    out << theme;
    writeThemeYaml(name, QString::fromUtf8(out.c_str()));
}

/// @brief Returns a name no theme holds yet, counting up from "<name> (copy)".
QString unusedThemeName(const QString& name)
{
    const QStringList taken = builtinThemeNames() + userThemeNames();
    QString candidate = name + QStringLiteral(" (copy)");
    for (int suffix = 2; taken.contains(candidate); ++suffix)
        candidate = name + QStringLiteral(" (copy %1)").arg(suffix);
    return candidate;
}

} // namespace

ThemeViewModel::ThemeViewModel(QObject* parent) : QObject(parent)
{
    writtenYaml_ = readThemeYaml(activeThemeName());
}

QVariantList ThemeViewModel::themes() const
{
    const QStringList names = builtinThemeNames() + userThemeNames();

    QVariantList themes;
    for (const QString& name : names)
        themes.append(QVariantMap{
            {"name", name},
            {"editable", !isBuiltinTheme(name)},
            {"swatch", themeSwatch(readThemeYaml(name))},
        });
    return themes;
}

QString ThemeViewModel::currentTheme() const { return activeThemeName(); }

bool ThemeViewModel::editable() const { return !isBuiltinTheme(currentTheme()); }

QVariantList ThemeViewModel::sections() const
{
    QQmlPropertyMap* variables = liveVariables();
    if (!variables) return {};

    QVariantList sections;
    for (const PaletteSection& section : paletteSections())
    {
        QVariantList entries;
        for (const auto& [name, label] : section.entries)
        {
            const QVariant value = variables->value(name);
            if (!value.isValid()) continue;
            entries.append(QVariantMap{
                {"name", name},
                {"label", label},
                {"kind", kindOf(value)},
            });
        }
        sections.append(QVariantMap{{"title", section.title}, {"entries", entries}});
    }
    return sections;
}

QStringList ThemeViewModel::modified() const { return QStringList(pending_.keyBegin(), pending_.keyEnd()); }

bool ThemeViewModel::hasPendingChanges() const { return !pending_.isEmpty(); }

QStringList ThemeViewModel::fontFamilies() const { return fontFamilies_; }

void ThemeViewModel::setFontFamilies(const QStringList& families) { fontFamilies_ = families; }

void ThemeViewModel::selectTheme(const QString& name)
{
    if (name == currentTheme()) return;
    switchToTheme(name);
}

void ThemeViewModel::setValue(const QString& name, const QVariant& value)
{
    if (!editable()) return;
    pending_.insert(name, value);
    preview();
    Q_EMIT modifiedChanged();
}

void ThemeViewModel::apply()
{
    if (pending_.isEmpty()) return;
    writtenYaml_ = currentYaml();
    writeThemeYaml(currentTheme(), writtenYaml_);
    pending_.clear();
    Q_EMIT modifiedChanged();
    Q_EMIT themesChanged();
}

void ThemeViewModel::revert()
{
    if (pending_.isEmpty()) return;
    pending_.clear();
    preview();
    Q_EMIT modifiedChanged();
}

QString ThemeViewModel::duplicateTheme()
{
    const QString name = unusedThemeName(currentTheme());
    writeThemeAs(name, currentYaml());
    switchToTheme(name);
    return name;
}

void ThemeViewModel::renameTheme(const QString& name)
{
    const QString previous = currentTheme();
    if (!editable() || name.isEmpty() || name == previous) return;

    writeThemeAs(name, currentYaml());
    deleteThemeFile(previous);
    switchToTheme(name);
}

void ThemeViewModel::deleteTheme()
{
    if (!editable()) return;
    deleteThemeFile(currentTheme());
    switchToTheme(defaultThemeName());
}

void ThemeViewModel::openThemeFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(themesDir()));
}

void ThemeViewModel::switchToTheme(const QString& name)
{
    pending_.clear();
    setActiveThemeName(name);
    writtenYaml_ = readThemeYaml(name);
    preview();
    Q_EMIT themesChanged();
    Q_EMIT currentThemeChanged();
    Q_EMIT sectionsChanged();
    Q_EMIT modifiedChanged();
}

void ThemeViewModel::preview()
{
    if (Theme* theme = Theme::instance()) theme->reload(currentYaml());
}

QString ThemeViewModel::currentYaml() const
{
    YAML::Node theme = writtenYaml_.isEmpty() ? YAML::Node(YAML::NodeType::Map)
                                              : YAML::Load(writtenYaml_.toStdString());
    if (!theme[kNameKey]) theme[kNameKey] = currentTheme().toStdString();

    for (auto edit = pending_.constBegin(); edit != pending_.constEnd(); ++edit)
        theme[kVariablesKey][edit.key().toStdString()] = yamlScalar(edit.value());

    YAML::Emitter out;
    out << theme;
    return QString::fromUtf8(out.c_str());
}

} // namespace enzo::ui
