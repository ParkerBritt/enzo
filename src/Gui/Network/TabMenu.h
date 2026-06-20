#pragma once

#include "Gui/UtilWidgets/PopupList.h"

#include <QLineEdit>

class NetworkPanel;

namespace enzo::ui {

/**
 * @brief Search menu that creates a node in the network from the operator table.
 *
 * The menu opens at the cursor with a search field above the list and filters the
 * operators as the user types.
 */
class TabMenu : public PopupList
{
    Q_OBJECT
  public:
    TabMenu(NetworkPanel* network);

    /// @brief Opens the menu at the cursor offset by the given amount.
    void showOnMouse(float dx = 0, float dy = 0);

  protected:
    int headerHeight() const override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    void applyFilter(const QString& text);
    void createNode(const std::string& nodeName);

    NetworkPanel* network_;
    QLineEdit* searchBar_;
};

} // namespace enzo::ui
