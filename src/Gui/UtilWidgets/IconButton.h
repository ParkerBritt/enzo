#pragma once

#include <QPushButton>
#include <qtmetamacros.h>
#include <string>

namespace enzo::ui {

/**
 * @brief Flat icon button for the small tool strips across the editor.
 *
 * The icon rests dimmed and brightens to full strength while hovered, growing
 * slightly so it reads as live under the cursor. The icon is pulled from the
 * registry and tinted, so a name is all the caller supplies.
 */
class IconButton : public QPushButton
{
    Q_OBJECT
  public:
    IconButton(const std::string& iconName, QWidget* parent = nullptr);

  protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

  private:
    bool hovered_ = false;
};

} // namespace enzo::ui
