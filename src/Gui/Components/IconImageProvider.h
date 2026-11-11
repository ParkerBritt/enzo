#pragma once

#include <QColor>
#include <QImage>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>

namespace enzo::ui {

/// @brief Returns the icon of this name drawn in one colour at a pixel size.
///
/// @return The image, null when no icon has the name or it fails to draw.
/// @note An invalid colour leaves the icon in its own colours.
QImage renderIcon(const QString& name, const QColor& color, const QSize& pixelSize);

/// @brief The image provider for icons tinted by name and colour.
///
/// `image://icon/plus?color=%23ffffff` gives the plus icon in white, and
/// `image://icon/node/enzo::grid` gives the grid node's icon in its own colours.
class IconImageProvider : public QQuickImageProvider
{
  public:
    IconImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};

} // namespace enzo::ui
