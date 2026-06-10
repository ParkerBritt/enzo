#pragma once

#include "Gui/UtilWidgets/PopupList.h"

#include <functional>
#include <vector>

namespace enzo::ui {

/**
 * @brief Nested menu built on the animated PopupList.
 *
 * A menu holds a tree of entries where a leaf carries an action and a branch
 * carries children. Branch rows show a chevron and reveal a child menu to their
 * right on hover or the Right key. Choosing a leaf runs its action and folds the
 * whole chain away together. A click outside or the Escape key dismisses it.
 *
 * e.g.
 *   menu->setEntries({
 *       {.text = "New", .action = onNew},
 *       {.text = "Import", .children = {{.text = "Enzo", .action = onImport}}},
 *   });
 */
class Menu : public PopupList
{
    Q_OBJECT
  public:
    struct Entry
    {
        QIcon icon;
        QString text;

        // Runs when a leaf row is chosen. Branch rows leave this empty.
        std::function<void()> action;

        // A non empty list turns the row into a branch opening a child menu.
        std::vector<Entry> children;
    };

    Menu(QWidget* parent = nullptr);
    ~Menu() override;

    /// @brief Replaces the menu contents with a tree of entries.
    void setEntries(std::vector<Entry> entries);

    /// @brief Opens the menu with its top left at a global position.
    /// @param takeFocus Hands keyboard focus to the menu. A child opened on hover leaves
    ///        focus with the parent until the cursor moves onto it.
    void popup(const QPoint& globalTopLeft, bool takeFocus = true);

  protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void onRowActivated(int position) override;
    void onHighlightChanged(int position, bool fromPointer) override;
    void paintRowDecoration(QPainter& painter, int position, const QRect& row) override;

  private:
    const Entry& entryAt(int position) const;
    bool isBranch(int position) const;

    void openSubmenu(int position, bool takeFocus);
    void closeSubmenu();
    void onSubmenuClosed();

    /// @brief Drives the hover on whichever level the cursor sits over.
    void routeHover(const QPoint& globalPos);

    /// @brief Width that fits the rows plus room for a branch chevron.
    int menuWidth() const;

    std::vector<Entry> entries_;

    // The child menu is created once and reused as the highlight moves between
    // branch rows. parentMenu_ lets a child hand focus back on the Left key.
    Menu* submenu_ = nullptr;
    Menu* parentMenu_ = nullptr;
    int submenuPosition_ = -1;

    // A parent folding its child away on purpose skips the upward cascade that a
    // child closing on its own would trigger.
    bool suppressCascade_ = false;
};

} // namespace enzo::ui
