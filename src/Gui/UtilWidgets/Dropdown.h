#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>
#include <qtmetamacros.h>
#include <vector>

namespace enzo::ui {

class DropdownPopup;

/**
 * @brief Reusable dropdown styled to match the other util widgets with a rounded
 * border and dark palette. Clicking the box reveals a popup list below it that
 * unrolls vertically to its full height.
 */
class Dropdown : public QWidget
{
    Q_OBJECT
  public:
    Dropdown(QWidget* parent = nullptr);

    void addItem(const QString& text);

    int currentIndex() const { return currentIndex_; }
    QString currentText() const;
    void setCurrentIndex(int index);

    QSize sizeHint() const override;

  Q_SIGNALS:
    void currentIndexChanged(int index);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

  private:
    struct Item
    {
        QIcon icon;
        QString text;
    };

    void openPopup();

    std::vector<Item> items_;
    int currentIndex_ = -1;
    bool popupOpen_ = false;

    DropdownPopup* popup_ = nullptr;

    friend class DropdownPopup;
};

/**
 * @brief Floating list shown beneath a Dropdown. Lives as a top level popup so it
 * can overlay arbitrary content. The list is custom painted and clipped to an
 * animated reveal height so it unrolls into view rather than resizing its window.
 */
class DropdownPopup : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int revealedHeight READ revealedHeight WRITE setRevealedHeight)
  public:
    DropdownPopup(Dropdown* owner);

    void openBeneath(QWidget* anchor, int selectedIndex);

  Q_SIGNALS:
    void itemSelected(int index);
    void closed();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

  private:
    void animateClose();
    int contentHeight() const;
    int rowAt(int localY) const;
    int maxScrollOffset() const;

    int revealedHeight() const { return revealedHeight_; }
    void setRevealedHeight(int height)
    {
        revealedHeight_ = height;
        update();
    }

    Dropdown* owner_;
    int hoveredIndex_ = -1;
    int selectedIndex_ = -1;
    int scrollOffset_ = 0;
    int revealedHeight_ = 0;
    bool closing_ = false;
};

} // namespace enzo::ui
