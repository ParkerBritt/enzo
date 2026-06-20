#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>
#include <qtmetamacros.h>
#include <vector>

class QPropertyAnimation;
class QPainter;

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
 *
 * Selection, highlight changes, and per row decoration are routed through virtual
 * hooks so a subclass such as Menu can layer nested menus on top without touching
 * the geometry and animation that live here.
 */
class PopupList : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int revealedHeight READ revealedHeight WRITE setRevealedHeight)
    Q_PROPERTY(qreal fadeElapsed READ fadeElapsed WRITE setFadeElapsed)
    Q_PROPERTY(qreal highlightTop READ highlightTop WRITE setHighlightTop)
    Q_PROPERTY(int scrollOffset READ scrollOffset WRITE setScrollOffset)
  public:
    struct Item
    {
        QIcon icon;
        QString text;
        QString data;
    };

    PopupList(QWidget* parent = nullptr);

    void addItem(const Item& item);

    /// @brief Adds an iconless row showing only text.
    void addItem(const QString& text);

    void clearItems();

    /// @brief Reveals the list anchored at a global position.
    /// @note selectedPosition is the row highlighted on open.
    void open(const QPoint& globalTopLeft, int width, int selectedPosition = 0);

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

    /// @brief Hook run when a row is chosen by click or Enter.
    /// @note The base reports the selection through itemSelected and closes the list.
    virtual void onRowActivated(int position);

    /// @brief Hook run when the highlighted row changes.
    /// @note fromPointer marks a hover move apart from keyboard navigation.
    virtual void onHighlightChanged(int position, bool fromPointer) {}

    /// @brief Hook to paint extra decoration on a row such as a submenu chevron.
    /// @note The row rectangle is given in list local coordinates.
    virtual void paintRowDecoration(QPainter& painter, int position, const QRect& row) {}

    /// @brief Local rectangle of a visible row including the header offset and scroll.
    QRect rowRect(int position) const;

    /// @brief Width that fits the longest visible row.
    int preferredWidth() const;

    /// @brief Moves the highlight to the row at a local y, firing the highlight hook.
    void hoverRowAt(int localY);

    /// @brief Reveals the list at a global position with the given width.
    /// @param selectedPosition Position within the visible subset to highlight on open.
    /// @param takeFocus Hands keyboard focus to the list. A submenu opened on hover leaves
    ///        it with the parent until the cursor moves onto the child.
    void
    openList(const QPoint& globalTopLeft, int width, int selectedPosition, bool takeFocus = true);
    void animateClose();

    /// @brief Turns the staggered row fade on open off so rows appear at once.
    void setRowFadeEnabled(bool enabled) { rowFadeEnabled_ = enabled; }

    /// @brief Resets the visible subset to every item in declaration order.
    void showAllItems();

    /// @brief Moves the highlight and selection to a position in the visible subset.
    void setHighlightedPosition(int position);

    /// @brief Position of the highlighted row within the visible subset.
    int highlightedPosition() const { return selectedPosition_; }

    /// @brief Resizes an open list to fit the current visible subset.
    /// @note Pass animated to glide the height change rather than snapping.
    void fitToContents(bool animated = false);

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
    void animateScrollTo(int offset);
    void jumpScrollTo(int offset);

    /// @brief Routes a chosen row through onRowActivated once.
    void chooseRow(int position);

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

    int scrollOffset() const { return scrollOffset_; }
    void setScrollOffset(int offset)
    {
        scrollOffset_ = offset;
        update();
    }

    int hoveredPosition_ = -1;
    int selectedPosition_ = -1;
    int scrollOffset_ = 0;
    int scrollTarget_ = 0;
    int revealedHeight_ = 0;
    qreal fadeElapsed_ = 0;
    qreal highlightTop_ = 0;
    bool closing_ = false;
    bool rowFadeEnabled_ = true;

    QPropertyAnimation* hoverAnim_ = nullptr;
    QPropertyAnimation* scrollAnim_ = nullptr;
    QPropertyAnimation* backgroundResizeAnim_ = nullptr;
};

} // namespace enzo::ui
