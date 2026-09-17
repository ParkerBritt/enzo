#include "Gui/Components/IconCursor.h"
#include "Engine/Core/InstallPaths.h"

#include <QBuffer>
#include <QCursor>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QPixmap>
#include <QQuickWindow>

namespace enzo::ui {

namespace {

// The side length of a Lucide icon's view box.
constexpr qreal lucideViewSize = 24;

} // namespace

IconCursor::IconCursor(QQuickItem* parent) : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::NoButton);
    connect(this, &IconCursor::cursorChanged, this, &IconCursor::applyCursor);
    connect(this, &QQuickItem::windowChanged, this, &IconCursor::applyCursor);
}

void IconCursor::applyCursor()
{
    if (!active_ || name_.isEmpty())
    {
        unsetCursor();
        return;
    }

    const QString path = QString::fromStdString(
        (getStaticDir() / "icons" / "lucide" / (name_.toStdString() + ".svg")).string()
    );
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("IconCursor: failed to load %s", qPrintable(path));
        unsetCursor();
        return;
    }

    // Lucide strokes are currentColor, so the colour is baked into the markup.
    QByteArray svg = file.readAll();
    svg.replace("currentColor", color_.name().toUtf8());

    const qreal pixelRatio = window() ? window()->devicePixelRatio() : 1.0;
    const int pixelSize = qRound(size_ * pixelRatio);
    QBuffer buffer(&svg);
    QImageReader reader(&buffer, "svg");
    reader.setScaledSize(QSize(pixelSize, pixelSize));
    const QImage image = reader.read();
    if (image.isNull())
    {
        qWarning("IconCursor: failed to render %s", qPrintable(path));
        unsetCursor();
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(pixelRatio);

    const qreal unitScale = size_ / lucideViewSize;
    setCursor(QCursor(pixmap, qRound(hotSpot_.x() * unitScale), qRound(hotSpot_.y() * unitScale)));
}

} // namespace enzo::ui
