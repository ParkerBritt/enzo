#include "Gui/Style/Theme.h"
#include "Engine/Core/InstallPaths.h"
#include "Gui/Style/ThemeLoader.h"
#include "Gui/Style/ThemeStore.h"

#include <QUrl>

#include <exception>

namespace enzo::ui {

namespace {

Theme* themeInstance = nullptr;

QString iconsDir()
{
    const QString dir = QString::fromStdString((getStaticDir() / "icons" / "lucide").string());
    return QUrl::fromLocalFile(dir + QChar(u'/')).toString();
}

} // namespace

Theme::Theme(QObject* parent) : QQmlPropertyMap(this, parent)
{
    themeInstance = this;
    insert(QStringLiteral("iconsDir"), iconsDir());
    reload(readThemeYaml(activeThemeName()));
}

Theme* Theme::instance() { return themeInstance; }

void Theme::reload(const QString& themeYaml)
{
    QHash<QString, QVariant> tokens;
    try
    {
        tokens = ThemeLoader::loadFromString(baseThemeYaml(), themeYaml);
    }
    catch (const std::exception& error)
    {
        qCritical("theme failed to load, %s", error.what());
        return;
    }

    // Each token is keyed group.slot. Nest its value under the group's map.
    for (const QString& key : tokens.keys())
    {
        const QString group = key.section('.', 0, 0);
        const QString slot = key.section('.', 1);
        getThemePropertyMap(group)->insert(slot, tokens.value(key));
    }
}

QQmlPropertyMap* Theme::getThemePropertyMap(const QString& group)
{
    // Get cached property map
    const QVariant storedVariant = value(group);
    QObject* storedObject = storedVariant.value<QObject*>();
    QQmlPropertyMap* existingMap = qobject_cast<QQmlPropertyMap*>(storedObject);
    if (existingMap) return existingMap;

    // Create map if one doesn't exist
    QQmlPropertyMap* createdMap = new QQmlPropertyMap(this);
    QObject* createdObject = createdMap;
    insert(group, QVariant::fromValue(createdObject));
    return createdMap;
}

} // namespace enzo::ui
