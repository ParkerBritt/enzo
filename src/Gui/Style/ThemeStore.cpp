#include "Gui/Style/ThemeStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#include <vector>

#include <yaml-cpp/yaml.h>

namespace enzo::ui {

namespace {

constexpr auto kThemeResourceDir = ":/theme";
constexpr auto kDefaultThemeFile = "default.yml";
constexpr auto kThemeSuffix = ".yml";
constexpr auto kThemeFilter = "*.yml";
constexpr auto kActiveThemeSetting = "theme";

/// @brief A theme compiled into the program.
struct BuiltinTheme
{
    QString name;
    QString yaml;
};

QString readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(file.readAll());
}

/// @brief Returns a theme file's name without its suffix.
QString themeNameFromFile(const QString& file)
{
    return file.chopped(QString::fromUtf8(kThemeSuffix).size());
}

/// @brief Returns the name a theme's YAML gives itself, or the fallback when it gives none.
QString titleOf(const QString& yaml, const QString& fallback)
{
    const YAML::Node theme = YAML::Load(yaml.toStdString());
    const YAML::Node title = theme["name"];
    return title ? QString::fromStdString(title.Scalar()) : fallback;
}

/// @brief Returns every theme compiled into the program, the default first and the rest by name.
const std::vector<BuiltinTheme>& builtinThemes()
{
    static const std::vector<BuiltinTheme> themes = [] {
        const QDir resources(QString::fromUtf8(kThemeResourceDir));
        QStringList files =
            resources.entryList({QString::fromUtf8(kThemeFilter)}, QDir::Files, QDir::Name);
        files.removeAll(QString::fromUtf8(kDefaultThemeFile));
        files.prepend(QString::fromUtf8(kDefaultThemeFile));

        std::vector<BuiltinTheme> loaded;
        for (const QString& file : files)
        {
            const QString yaml = readFile(resources.filePath(file));
            loaded.push_back({titleOf(yaml, themeNameFromFile(file)), yaml});
        }
        return loaded;
    }();
    return themes;
}

QString themePath(const QString& name) { return themesDir() + QChar(u'/') + name + kThemeSuffix; }

} // namespace

QString themesDir()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const QString dir = root + QChar(u'/') + QCoreApplication::applicationName()
                        + QStringLiteral("/themes");
    QDir().mkpath(dir);
    return dir;
}

QString baseThemeYaml() { return builtinThemes().front().yaml; }

QString defaultThemeName() { return builtinThemes().front().name; }

QStringList builtinThemeNames()
{
    QStringList names;
    for (const BuiltinTheme& theme : builtinThemes())
        names << theme.name;
    return names;
}

bool isBuiltinTheme(const QString& name)
{
    for (const BuiltinTheme& theme : builtinThemes())
        if (theme.name == name) return true;
    return false;
}

QStringList userThemeNames()
{
    QStringList names;
    const QDir dir(themesDir());
    for (const QString& file : dir.entryList({QString::fromUtf8(kThemeFilter)}, QDir::Files))
        names << themeNameFromFile(file);
    names.sort(Qt::CaseInsensitive);
    return names;
}

QString activeThemeName()
{
    const QString name = QSettings().value(kActiveThemeSetting).toString();
    if (name.isEmpty()) return defaultThemeName();
    if (!isBuiltinTheme(name) && !userThemeNames().contains(name)) return defaultThemeName();
    return name;
}

void setActiveThemeName(const QString& name) { QSettings().setValue(kActiveThemeSetting, name); }

QString readThemeYaml(const QString& name)
{
    // Leaves the default theme empty, since it is the base the others lay over.
    if (name == defaultThemeName()) return QString();

    for (const BuiltinTheme& theme : builtinThemes())
        if (theme.name == name) return theme.yaml;

    return readFile(themePath(name));
}

void writeThemeYaml(const QString& name, const QString& yaml)
{
    QFile file(themePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream(&file) << yaml;
}

void deleteThemeFile(const QString& name) { QFile::remove(themePath(name)); }

} // namespace enzo::ui
