#include "Gui/Icons/Builtin.h"

#include "Gui/IconRegistry.h"

#include <QCoreApplication>
#include <QDir>
#include <QString>

namespace enzo::ui::icons
{

namespace
{

QString resolveIconsRoot()
{
    const QString installed = QCoreApplication::applicationDirPath()
        + "/../share/icons";
    if (QDir(installed).exists()) return installed;

    const QString dev = QStringLiteral(ENZO_DEV_ICONS_DIR);
    if (!dev.isEmpty() && QDir(dev).exists()) return dev;

    return {};
}

}

void registerBuiltins()
{
    auto& registry = IconRegistry::instance();
    const QString root = resolveIconsRoot();
    if (root.isEmpty()) return;

    const QFileInfoList subdirs = QDir(root)
        .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& subdir : subdirs)
    {
        registry.registerDirectory("", subdir.absoluteFilePath());
    }
}

}
