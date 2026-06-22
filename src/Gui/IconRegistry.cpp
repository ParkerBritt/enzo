#include "Gui/IconRegistry.h"
#include "Gui/Style.h"

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QPixmap>

#include <algorithm>

namespace enzo::ui {

namespace {

QIcon makeFallbackIcon()
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(enzo::style::color::error);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(2, 2, 28, 28);

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(18);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "?");

    return QIcon(pixmap);
}

} // namespace

IconRegistry& IconRegistry::instance()
{
    static IconRegistry registry;
    return registry;
}

IconRegistry::IconRegistry() : fallback_(makeFallbackIcon()) {}

void IconRegistry::registerIcon(const std::string& name, QIcon icon)
{
    icons_.emplace(name, std::move(icon));
}

void IconRegistry::registerIcon(const std::string& name, const QString& resourcePath)
{
    icons_.emplace(name, QIcon(resourcePath));
}

int IconRegistry::registerDirectory(const std::string& prefix, const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) return 0;

    const QFileInfoList entries = dir.entryInfoList({"*.svg"}, QDir::Files);
    for (const QFileInfo& entry : entries)
    {
        const std::string name = prefix + entry.completeBaseName().toStdString();
        icons_.emplace(name, QIcon(entry.absoluteFilePath()));
    }
    return entries.size();
}

QPixmap IconRegistry::pixmap(const std::string& name, QSize size, QColor color, float opacity) const
{
    auto it = icons_.find(name);
    const QIcon& source = (it != icons_.end()) ? it->second : fallback_;

    QPixmap result = source.pixmap(size);
    if (result.isNull()) return result;

    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

    if (color.isValid())
    {
        QColor fill = color;
        fill.setAlphaF(fill.alphaF() * clampedOpacity);
        QPainter painter(&result);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(result.rect(), fill);
    }
    else if (clampedOpacity < 1.0f)
    {
        QPixmap faded(result.size());
        faded.fill(Qt::transparent);
        QPainter painter(&faded);
        painter.setOpacity(clampedOpacity);
        painter.drawPixmap(0, 0, result);
        result = faded;
    }

    return result;
}

QIcon IconRegistry::lookup(const std::string& name, QColor color, float opacity) const
{
    static constexpr int BAKE_SIZE = 64;
    return QIcon(pixmap(name, QSize(BAKE_SIZE, BAKE_SIZE), color, opacity));
}

} // namespace enzo::ui
