#include "Gui/UtilWidgets/PopupList.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QWheelEvent>
#include <algorithm>
#include <numeric>

namespace {

constexpr int itemHeight = 24;
constexpr int padding = 4;
constexpr int borderRadius = 8;
constexpr int maxPopupHeight = 240;
constexpr int textPadding = 10;
constexpr int iconSize = 14;
constexpr int iconTextGap = 6;
constexpr qreal itemFadeMs = 300;
constexpr qreal itemStaggerMs = 30;
constexpr int scrollbarWidth = 3;
constexpr int scrollbarMargin = 2;

const QColor borderColor("#383838");
const QColor backgroundColor(25, 25, 25);
const QColor textColor("#B3B3B3");
const QColor selectedTextColor("#E6E6E6");
const QColor hoverColor(60, 60, 60, 153);

} // namespace

enzo::ui::PopupList::PopupList(QWidget* parent) : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    hoverAnim_ = new QPropertyAnimation(this, "highlightTop", this);
    hoverAnim_->setDuration(130);
    hoverAnim_->setEasingCurve(QEasingCurve::OutCubic);

    scrollAnim_ = new QPropertyAnimation(this, "scrollOffset", this);
    scrollAnim_->setDuration(180);
    scrollAnim_->setEasingCurve(QEasingCurve::OutCubic);

    backgroundResizeAnim_ = new QPropertyAnimation(this, "revealedHeight", this);
    backgroundResizeAnim_->setDuration(180);
    backgroundResizeAnim_->setEasingCurve(QEasingCurve::OutCubic);
    // Settle the window onto the final box height once the resize lands
    connect(backgroundResizeAnim_, &QPropertyAnimation::finished, this, [this] {
        resize(width(), headerHeight() + revealedHeight_);
    });
}

void enzo::ui::PopupList::addItem(const Item& item)
{
    items_.push_back(item);
}

void enzo::ui::PopupList::clearItems()
{
    items_.clear();
    visibleIndices_.clear();
}

void enzo::ui::PopupList::showAllItems()
{
    visibleIndices_.resize(items_.size());
    std::iota(visibleIndices_.begin(), visibleIndices_.end(), 0);
}

void enzo::ui::PopupList::setHighlightedPosition(int position)
{
    if (visibleIndices_.empty())
    {
        selectedPosition_ = -1;
        hoveredPosition_ = -1;
    }
    else
    {
        selectedPosition_ = std::clamp(position, 0, static_cast<int>(visibleIndices_.size()) - 1);
        hoveredPosition_ = selectedPosition_;
    }
    jumpScrollTo(0);
    hoverAnim_->stop();
    highlightTop_ = padding + std::max(0, selectedPosition_) * itemHeight;
    update();
}

void enzo::ui::PopupList::fitToContents(bool animated)
{
    // An empty subset collapses the list so only the header remains
    const int target = visibleIndices_.empty() ? 0 : std::min(contentHeight(), maxPopupHeight);

    if (!animated)
    {
        backgroundResizeAnim_->stop();
        resize(width(), headerHeight() + target);
        revealedHeight_ = target;
        update();
        return;
    }

    // Hold the window at the taller height so the box can paint while it animates
    resize(width(), headerHeight() + std::max(revealedHeight_, target));
    backgroundResizeAnim_->stop();
    backgroundResizeAnim_->setStartValue(revealedHeight_);
    backgroundResizeAnim_->setEndValue(target);
    backgroundResizeAnim_->start();
}

int enzo::ui::PopupList::contentHeight() const
{
    return static_cast<int>(visibleIndices_.size()) * itemHeight + padding * 2;
}

int enzo::ui::PopupList::listHeight() const
{
    return height() - headerHeight();
}

int enzo::ui::PopupList::maxScrollOffset() const
{
    return std::max(0, contentHeight() - listHeight());
}

int enzo::ui::PopupList::rowAt(int widgetY) const
{
    const int localY = widgetY - headerHeight();
    if (localY < 0 || localY > revealedHeight_) return -1;
    const int position = (localY - padding + scrollOffset_) / itemHeight;
    if (position < 0 || position >= static_cast<int>(visibleIndices_.size())) return -1;
    return position;
}

void enzo::ui::PopupList::openList(const QPoint& globalTopLeft, int width, int selectedPosition)
{
    selectedPosition_ = selectedPosition;
    hoveredPosition_ = selectedPosition;
    jumpScrollTo(0);
    closing_ = false;

    const int fullHeight = std::min(contentHeight(), maxPopupHeight);

    // Highlight slides into the current selection from one row above
    const qreal highlightTarget = padding + std::max(0, selectedPosition) * itemHeight;
    hoverAnim_->stop();
    highlightTop_ = highlightTarget - itemHeight;
    hoverAnim_->setStartValue(highlightTop_);
    hoverAnim_->setEndValue(highlightTarget);
    hoverAnim_->start();

    setGeometry(globalTopLeft.x(), globalTopLeft.y(), width, headerHeight() + fullHeight);

    revealedHeight_ = 0;
    fadeElapsed_ = 0;
    show();
    setFocus();

    // Unroll the list to its full height
    backgroundResizeAnim_->stop();
    backgroundResizeAnim_->setStartValue(0);
    backgroundResizeAnim_->setEndValue(fullHeight);
    backgroundResizeAnim_->start();

    // Fade each row in turn from the top
    const size_t rowCount = visibleIndices_.size();
    const qreal fadeTotal = itemStaggerMs * (rowCount == 0 ? 0 : rowCount - 1) + itemFadeMs;
    auto fade = new QPropertyAnimation(this, "fadeElapsed", this);
    fade->setDuration(static_cast<int>(fadeTotal));
    fade->setStartValue(0.0);
    fade->setEndValue(fadeTotal);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void enzo::ui::PopupList::ensureVisible(int position)
{
    // Measure against the target so repeated steps walk past the fold smoothly
    int desired = scrollTarget_;
    const int top = padding + position * itemHeight;
    const int bottom = top + itemHeight;
    if (top - desired < padding)
    {
        desired = top - padding;
    }
    else if (bottom - desired > listHeight() - padding)
    {
        desired = bottom - listHeight() + padding;
    }
    animateScrollTo(desired);
}

void enzo::ui::PopupList::animateHighlightTo(int position)
{
    hoverAnim_->stop();
    hoverAnim_->setStartValue(highlightTop_);
    hoverAnim_->setEndValue(padding + position * itemHeight);
    hoverAnim_->start();
}

void enzo::ui::PopupList::animateScrollTo(int offset)
{
    scrollTarget_ = std::clamp(offset, 0, maxScrollOffset());
    if (scrollTarget_ == scrollOffset_) return;
    scrollAnim_->stop();
    scrollAnim_->setStartValue(scrollOffset_);
    scrollAnim_->setEndValue(scrollTarget_);
    scrollAnim_->start();
}

void enzo::ui::PopupList::jumpScrollTo(int offset)
{
    scrollAnim_->stop();
    scrollOffset_ = std::clamp(offset, 0, maxScrollOffset());
    scrollTarget_ = scrollOffset_;
}

void enzo::ui::PopupList::moveHighlight(int delta)
{
    if (visibleIndices_.empty()) return;
    const int next =
        std::clamp(selectedPosition_ + delta, 0, static_cast<int>(visibleIndices_.size()) - 1);
    if (next == selectedPosition_) return;
    selectedPosition_ = next;
    hoveredPosition_ = next;
    ensureVisible(next);
    animateHighlightTo(next);
}

void enzo::ui::PopupList::confirmSelection()
{
    if (selectedPosition_ < 0 || selectedPosition_ >= static_cast<int>(visibleIndices_.size()))
        return;
    Q_EMIT itemSelected(visibleIndices_[selectedPosition_]);
    animateClose();
}

void enzo::ui::PopupList::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // A collapsed list draws nothing so no border seam shows beneath the header
    if (revealedHeight_ <= 0) return;

    painter.translate(0, headerHeight());

    // Grow the rounded box with the reveal so the bottom edge stays clean
    QRectF box = QRectF(0, 0, width(), revealedHeight_).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath clip;
    clip.addRoundedRect(box, borderRadius, borderRadius);
    painter.setClipPath(clip);

    painter.fillRect(box, backgroundColor);

    // Highlight slides between rows, leaving an even gap from the scrollbar when present
    const int rightInset =
        maxScrollOffset() > 0 ? padding + scrollbarMargin + scrollbarWidth : padding;
    QRect highlight(0, static_cast<int>(highlightTop_) - scrollOffset_, width(), itemHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(hoverColor);
    painter.drawRoundedRect(highlight.adjusted(padding, 1, -rightInset, -1), 5, 5);

    // Rows, translated by the scroll offset and naturally clipped to the reveal
    for (int position = 0; position < static_cast<int>(visibleIndices_.size()); ++position)
    {
        const Item& item = items_[visibleIndices_[position]];
        const int top = padding + position * itemHeight - scrollOffset_;
        if (top + itemHeight < 0 || top > revealedHeight_) continue;

        QRect row(0, top, width(), itemHeight);
        const qreal opacity =
            std::clamp((fadeElapsed_ - position * itemStaggerMs) / itemFadeMs, 0.0, 1.0);
        painter.setOpacity(opacity);

        // Icon sits at the left edge and the text follows when one is present
        int textLeft = textPadding;
        if (!item.icon.isNull())
        {
            const int iconTop = top + (itemHeight - iconSize) / 2;
            painter.drawPixmap(
                QRect(textPadding, iconTop, iconSize, iconSize),
                item.icon.pixmap(iconSize, iconSize)
            );
            textLeft = textPadding + iconSize + iconTextGap;
        }

        painter.setPen(position == selectedPosition_ ? selectedTextColor : textColor);
        painter.drawText(
            row.adjusted(textLeft, 0, -textPadding, 0),
            Qt::AlignVCenter | Qt::AlignLeft,
            item.text
        );
        painter.setOpacity(1.0);
    }

    // Soften the bottom edge when rows continue below the fold
    if (scrollOffset_ < maxScrollOffset())
    {
        QColor transparentBackground = backgroundColor;
        transparentBackground.setAlpha(0);
        QLinearGradient gradient(0, revealedHeight_ - itemHeight, 0, revealedHeight_);
        gradient.setColorAt(0.0, transparentBackground);
        gradient.setColorAt(1.0, backgroundColor);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRect(QRectF(0, revealedHeight_ - itemHeight, width(), itemHeight));
    }

    // Scrollbar indicator when the list overflows
    if (maxScrollOffset() > 0)
    {
        const float visibleFraction = static_cast<float>(listHeight()) / contentHeight();
        const int trackHeight = revealedHeight_ - padding * 2;
        const int thumbHeight = std::max(20, static_cast<int>(trackHeight * visibleFraction));
        const float scrollFraction = static_cast<float>(scrollOffset_) / maxScrollOffset();
        const int thumbTop =
            padding + static_cast<int>((trackHeight - thumbHeight) * scrollFraction);
        painter.setPen(Qt::NoPen);
        painter.setBrush(borderColor);
        painter.drawRoundedRect(
            QRect(
                width() - scrollbarMargin - scrollbarWidth,
                thumbTop,
                scrollbarWidth,
                thumbHeight
            ),
            1,
            1
        );
    }

    // Outline drawn last so it sits above the fill
    painter.setClipping(false);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, borderRadius, borderRadius);
}

void enzo::ui::PopupList::mouseMoveEvent(QMouseEvent* event)
{
    const int position = rowAt(event->pos().y());
    if (position < 0 || position == hoveredPosition_) return;
    hoveredPosition_ = position;
    selectedPosition_ = position;
    animateHighlightTo(position);
}

void enzo::ui::PopupList::mousePressEvent(QMouseEvent* event)
{
    const QPoint pos = event->pos();
    const bool insideBox =
        QRect(0, headerHeight(), width(), revealedHeight_).contains(pos);
    if (insideBox)
    {
        const int position = rowAt(pos.y());
        if (position >= 0)
        {
            selectedPosition_ = position;
            Q_EMIT itemSelected(visibleIndices_[position]);
        }
    }
    animateClose();
}

void enzo::ui::PopupList::mouseReleaseEvent(QMouseEvent* event)
{
    // Press and hold to open, drag, then release over an item to select
    if (closing_) return;
    const int position = rowAt(event->pos().y());
    if (position < 0) return;

    selectedPosition_ = position;
    Q_EMIT itemSelected(visibleIndices_[position]);
    animateClose();
}

void enzo::ui::PopupList::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        animateClose();
        return;
    case Qt::Key_Up:
        moveHighlight(-1);
        return;
    case Qt::Key_Down:
        moveHighlight(1);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        confirmSelection();
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

void enzo::ui::PopupList::wheelEvent(QWheelEvent* event)
{
    if (maxScrollOffset() == 0) return;
    // Accumulate against the target so successive ticks glide further rather than restart
    animateScrollTo(scrollTarget_ - event->angleDelta().y());
}

void enzo::ui::PopupList::animateClose()
{
    if (closing_) return;
    closing_ = true;
    Q_EMIT aboutToClose();

    backgroundResizeAnim_->stop();
    auto collapse = new QPropertyAnimation(this, "revealedHeight", this);
    collapse->setDuration(140);
    collapse->setEasingCurve(QEasingCurve::InCubic);
    collapse->setStartValue(revealedHeight_);
    collapse->setEndValue(0);
    connect(collapse, &QPropertyAnimation::finished, this, [this] { hide(); });
    collapse->start(QAbstractAnimation::DeleteWhenStopped);
}

void enzo::ui::PopupList::hideEvent(QHideEvent* event)
{
    closing_ = false;
    Q_EMIT closed();
    QWidget::hideEvent(event);
}
