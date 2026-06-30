#pragma once

#include <QQmlPropertyMap>
#include <qqmlregistration.h>

namespace enzo::ui {

/// @brief The YAML theme, exposed to QML as the `Theme` singleton.
class Theme : public QQmlPropertyMap
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

  public:
    explicit Theme(QObject* parent = nullptr);

  private:
    /// @brief Returns the style property to value map for a theme group like `parameter`.
    /// @note Creates the group's map on first use, caching it for future calls.
    QQmlPropertyMap* getThemePropertyMap(const QString& group);
};

} // namespace enzo::ui
