#include "Gui/Components/IconImageProvider.h"
#include "Gui/Components/IconFiles.h"

#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QUrlQuery>

namespace enzo::ui {

namespace {

constexpr int defaultPixelSize = 48;

} // namespace

QImage renderIcon(const QString& name, const QColor& color, const QSize& pixelSize)
{
    const QString path = QString::fromStdString(getIconFile(name).string());
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("Icon: no icon named %s", qPrintable(name));
        return {};
    }

    // Tints the currentColor strokes, which Lucide icons draw with.
    QByteArray svg = file.readAll();
    if (color.isValid()) svg.replace("currentColor", color.name().toUtf8());

    QBuffer buffer(&svg);
    QImageReader reader(&buffer, "svg");
    reader.setScaledSize(pixelSize);
    const QImage image = reader.read();
    if (image.isNull()) qWarning("Icon: failed to draw %s", qPrintable(path));
    return image;
}

IconImageProvider::IconImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage IconImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    const QString name = id.section('?', 0, 0);
    const QUrlQuery query(id.section('?', 1));
    const QColor color = QColor::fromString(query.queryItemValue("color", QUrl::FullyDecoded));

    const QSize pixelSize =
        requestedSize.isValid() ? requestedSize : QSize(defaultPixelSize, defaultPixelSize);
    const QImage image = renderIcon(name, color, pixelSize);
    if (size) *size = image.size();
    return image;
}

} // namespace enzo::ui
