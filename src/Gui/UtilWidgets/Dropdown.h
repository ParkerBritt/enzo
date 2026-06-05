#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>
#include <qtmetamacros.h>
#include <vector>

class QPropertyAnimation;

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
    Q_PROPERTY(qreal arrowRotation READ arrowRotation WRITE setArrowRotation)
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
    void animateArrow(qreal target);

    qreal arrowRotation() const { return arrowRotation_; }
    void setArrowRotation(qreal degrees)
    {
        arrowRotation_ = degrees;
        update();
    }

    std::vector<Item> items_;
    int currentIndex_ = -1;
    bool popupOpen_ = false;
    qreal arrowRotation_ = 0;

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
    Q_PROPERTY(qreal fadeElapsed READ fadeElapsed WRITE setFadeElapsed)
    Q_PROPERTY(qreal highlightTop READ highlightTop WRITE setHighlightTop)
  public:
    DropdownPopup(Dropdown* owner);

    void openBeneath(QWidget* anchor, int selectedIndex);

  Q_SIGNALS:
    void itemSelected(int index);
    void aboutToClose();
    void closed();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hideEvent(QHideEvent* event) override;

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

    qreal fadeElapsed() const { return fadeElapsed_; }
    void setFadeElapsed(qreal elapsed)
    {
        fadeElapsed_ = elapsed;
        update();
    }

    qreal highlightTop() const { return highlightTop_; }
    void setHighlightTop(qreal top)
    {
        highlightTop_ = top;
        update();
    }

    Dropdown* owner_;
    int hoveredIndex_ = -1;
    int selectedIndex_ = -1;
    int scrollOffset_ = 0;
    int revealedHeight_ = 0;
    qreal fadeElapsed_ = 0;
    qreal highlightTop_ = 0;
    bool closing_ = false;

    QPropertyAnimation* hoverAnim_ = nullptr;
};

} // namespace enzo::ui
