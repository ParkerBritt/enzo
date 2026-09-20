#include "Gui/Style/ThemeStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#include <yaml-cpp/yaml.h>

namespace enzo::ui {

namespace {

constexpr auto kShippedThemePath = ":/theme/default.yml";
constexpr auto kThemeSuffix = ".yml";
constexpr auto kActiveThemeSetting = "theme";

QString readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    return QString::fromUtf8(file.readAll());
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

QString shippedThemeName()
{
    static const QString name = [] {
        const YAML::Node theme = YAML::Load(shippedThemeYaml().toStdString());
        const YAML::Node title = theme["name"];
        return title ? QString::fromStdString(title.Scalar()) : QStringLiteral("Enzo");
    }();
    return name;
}

QString shippedThemeYaml()
{
    static const QString yaml = readFile(QString::fromUtf8(kShippedThemePath));
    return yaml;
}

QStringList userThemeNames()
{
    QStringList names;
    for (const QString& file : QDir(themesDir()).entryList({QStringLiteral("*.yml")}, QDir::Files))
        names << file.chopped(QString(kThemeSuffix).size());
    names.sort(Qt::CaseInsensitive);
    return names;
}

QString activeThemeName()
{
    const QString name = QSettings().value(kActiveThemeSetting).toString();
    if (name.isEmpty()) return shippedThemeName();
    if (name != shippedThemeName() && !userThemeNames().contains(name)) return shippedThemeName();
    return name;
}

void setActiveThemeName(const QString& name) { QSettings().setValue(kActiveThemeSetting, name); }

QString readThemeYaml(const QString& name)
{
    if (name == shippedThemeName()) return QString();
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
