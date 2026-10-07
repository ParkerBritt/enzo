#pragma once
#include "Engine/Core/Path.h"
#include <QString>
#include <QStringList>
#include <string>

namespace enzo::ui {

/**
 * @brief Returns the names along a path, from the top down.
 *
 * getPathNames("/assets/component") gives ["assets", "component"].
 */
inline QStringList getPathNames(const Path& path)
{
    QStringList names;
    for (const std::string& name : path.split())
        names.append(QString::fromStdString(name));
    return names;
}

} // namespace enzo::ui
