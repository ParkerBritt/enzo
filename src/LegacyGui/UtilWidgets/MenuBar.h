#pragma once

#include "LegacyGui/UtilWidgets/Menu.h"

#include <QString>
#include <QWidget>
#include <functional>
#include <vector>

namespace enzo::ui {

/**
 * @brief Horizontal bar of menu titles backed by the custom Menu popup.
 *
 * Behaves like a native menu bar. Pressing a title opens its menu beneath it,
 * pressing it again closes, and while a menu is open hovering another title
 * switches to it. The open Menu hands mouse events over the bar back here, so
 * hover and switching run through the ordinary event handlers.
 *
 * e.g.
 *   bar->addMenu("File", [this] { return fileEntries(); });
 */
class MenuBar : public QWidget
{
    Q_OBJECT
  public:
    MenuBar(QWidget* parent = nullptr);

    /// @brief Adds a title whose entries are built fresh each time it opens.
    void addMenu(const QString& title, std::function<std::vector<Menu::Entry>()> build);

    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    struct Item
    {
        QString title;
        std::function<std::vector<Menu::Entry>()> build;
    };

    /// @brief Local rectangle of a title.
    QRect titleRect(int index) const;

    /// @brief Index of the title under a local point, or -1 between titles.
    int titleAt(const QPoint& pos) const;

    /// @brief Opens an item's menu beneath its title.
    void openMenuAt(int index);

    std::vector<Item> items_;

    // One menu instance is repopulated by whichever title opens it
    Menu* menu_;

    int hoverIndex_ = -1;
    int openIndex_ = -1;
};

} // namespace enzo::ui
