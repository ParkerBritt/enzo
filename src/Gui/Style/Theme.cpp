#include "Gui/Style/Theme.h"
#include <QUrl>

namespace enzo::ui {

Theme::Theme(QObject* parent) : QObject(parent)
{
    iconsDir_ = QUrl::fromLocalFile(QStringLiteral(ENZO_DEV_STATIC_DIR "/icons/lucide/")).toString();
}

} // namespace enzo::ui
