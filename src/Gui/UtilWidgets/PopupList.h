#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>
#include <qtmetamacros.h>
#include <vector>

class QPropertyAnimation;

namespace enzo::ui {

/**
 * @brief Animated floating list shared by the parameter dropdown and the network
 * tab menu.
 *
 * Rows are custom painted and clipped to a reveal height so the list unrolls into
 * view. Only the items named in visibleIndices_ are drawn so a subclass can filter
 * the contents while keyboard navigation, selection, and scrolling stay in the
 * base. A subclass reserves a strip above the list for its own chrome by
 * overriding headerHeight.
 */
class PopupList : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int revealedHeight READ revealedHeight WRITE setRevealedHeight)
    Q_PROPERTY(qreal fadeElapsed READ fadeElapsed WRITE setFadeElapsed)
    Q_PROPERTY(qreal highlightTop READ highlightTop WRITE setHighlightTop)
  public:
    struct Item
    {
        QIcon icon;
        QString text;
        QString data;
    };

    PopupList(QWidget* parent = nullptr);

    void addItem(const Item& item);
    void clearItems();

  Q_SIGNALS:
    // The index identifies the chosen item within items_ rather than its position
    // in the visible subset.
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

    /// @brief Height reserved above the list for subclass chrome such as a search field.
    virtual int headerHeight() const { return 0; }

    /// @brief Reveals the list at a global position with the given width.
    /// @note selectedPosition is the position within the visible subset to highlight on open.
    void openList(const QPoint& globalTopLeft, int width, int selectedPosition);
    void animateClose();

    /// @brief Resets the visible subset to every item in declaration order.
    void showAllItems();

    /// @brief Moves the highlight and selection to a position in the visible subset.
    void setHighlightedPosition(int position);

    /// @brief Resizes an open list to fit the current visible subset.
    void fitToContents();

    int contentHeight() const;
    int listHeight() const;

    std::vector<Item> items_;
    std::vector<int> visibleIndices_;

  private:
    int rowAt(int widgetY) const;
    int maxScrollOffset() const;
    void moveHighlight(int delta);
    void confirmSelection();
    void ensureVisible(int position);
    void animateHighlightTo(int position);

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

    int hoveredPosition_ = -1;
    int selectedPosition_ = -1;
    int scrollOffset_ = 0;
    int revealedHeight_ = 0;
    qreal fadeElapsed_ = 0;
    qreal highlightTop_ = 0;
    bool closing_ = false;

    QPropertyAnimation* hoverAnim_ = nullptr;
};

} // namespace enzo::ui
