#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <string>
#include <unordered_map>

namespace enzo::ui {

class IconRegistry
{
  public:
    static IconRegistry& instance();

    void registerIcon(const std::string& name, QIcon icon);
    void registerIcon(const std::string& name, const QString& resourcePath);
    int registerDirectory(const std::string& prefix, const QString& dirPath);

    QIcon lookup(const std::string& name, QColor color = Qt::white, float opacity = 1.0f) const;
    QPixmap pixmap(
        const std::string& name,
        QSize size,
        QColor color = Qt::white,
        float opacity = 1.0f
    ) const;

  private:
    IconRegistry();

    std::unordered_map<std::string, QIcon> icons_;
    QIcon fallback_;
};

} // namespace enzo::ui
