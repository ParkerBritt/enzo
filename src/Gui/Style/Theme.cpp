#include "Gui/Style/Theme.h"
#include "Gui/Style/ThemeLoader.h"

#include <QStandardPaths>
#include <QUrl>

#include <exception>

namespace enzo::ui {

namespace {

QString defaultThemePath() { return QStringLiteral(ENZO_DEV_STATIC_DIR "/theme/default.yml"); }

QString userThemePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir.isEmpty() ? QString() : dir + QStringLiteral("/theme.yml");
}

QString iconsDir()
{
    return QUrl::fromLocalFile(QStringLiteral(ENZO_DEV_STATIC_DIR "/icons/lucide/")).toString();
}

} // namespace

Theme::Theme(QObject* parent) : QQmlPropertyMap(this, parent)
{
    QHash<QString, QVariant> tokens;
    try
    {
        tokens = ThemeLoader::loadFromFile(defaultThemePath(), userThemePath());
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

    insert(QStringLiteral("iconsDir"), iconsDir());
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
