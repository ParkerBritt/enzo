#include "Gui/UtilWidgets/Dropdown.h"
#include "Gui/IconRegistry.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QWheelEvent>
#include <algorithm>

namespace {

constexpr int itemHeight = 24;
constexpr int padding = 4;
constexpr int borderRadius = 8;
constexpr int maxPopupHeight = 240;
constexpr int textPadding = 10;
constexpr int arrowSize = 14;
constexpr int arrowMargin = 8;
constexpr int popupGap = 5;
constexpr qreal itemFadeMs = 300;
constexpr qreal itemStaggerMs = 30;

const QColor borderColor("#383838");
const QColor backgroundColor("#1a1a1a");
const QColor textColor("#B3B3B3");
const QColor selectedTextColor("#E6E6E6");
const QColor hoverColor(60, 60, 60, 153);

} // namespace

enzo::ui::Dropdown::Dropdown(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);

    popup_ = new DropdownPopup(this);
    connect(popup_, &DropdownPopup::itemSelected, this, &Dropdown::setCurrentIndex);
    connect(popup_, &DropdownPopup::aboutToClose, this, [this] { animateArrow(0); });
    connect(popup_, &DropdownPopup::closed, this, [this] {
        popupOpen_ = false;
        animateArrow(0);
    });
}

void enzo::ui::Dropdown::addItem(const QString& text)
{
    items_.push_back({QIcon(), text});
    if (currentIndex_ < 0) currentIndex_ = 0;
    updateGeometry();
    update();
}

QString enzo::ui::Dropdown::currentText() const
{
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(items_.size())) return {};
    return items_[currentIndex_].text;
}

void enzo::ui::Dropdown::setCurrentIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(items_.size())) return;
    if (index == currentIndex_) return;
    currentIndex_ = index;
    update();
    Q_EMIT currentIndexChanged(index);
}

QSize enzo::ui::Dropdown::sizeHint() const
{
    int widest = 0;
    for (const Item& item : items_)
    {
        widest = std::max(widest, fontMetrics().horizontalAdvance(item.text));
    }
    const int width = textPadding * 2 + widest + arrowSize + arrowMargin;
    return {width, itemHeight};
}

void enzo::ui::Dropdown::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Box outline matching the other util widgets
    QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, borderRadius, borderRadius);

    // Current selection text
    painter.setPen(textColor);
    QRect textRect = rect().adjusted(textPadding, 0, -(arrowSize + arrowMargin), 0);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, currentText());

    // Indicator chevron pulled from the icon registry, rotated while the popup is open
    const QPixmap chevron =
        IconRegistry::instance().pixmap("chevron-down", QSize(arrowSize, arrowSize), textColor);
    const QPointF chevronCenter(width() - arrowMargin - arrowSize / 2.0, height() / 2.0);
    painter.save();
    painter.translate(chevronCenter);
    painter.rotate(arrowRotation_);
    painter.drawPixmap(QPointF(-arrowSize / 2.0, -arrowSize / 2.0), chevron);
    painter.restore();
}

void enzo::ui::Dropdown::mousePressEvent(QMouseEvent*)
{
    if (!popupOpen_) openPopup();
}

void enzo::ui::Dropdown::openPopup()
{
    if (items_.empty()) return;
    popupOpen_ = true;
    animateArrow(180);
    popup_->openBeneath(this, currentIndex_);
}

void enzo::ui::Dropdown::animateArrow(qreal target)
{
    auto spin = new QPropertyAnimation(this, "arrowRotation", this);
    spin->setDuration(180);
    spin->setEasingCurve(QEasingCurve::OutCubic);
    spin->setStartValue(arrowRotation_);
    spin->setEndValue(target);
    spin->start(QAbstractAnimation::DeleteWhenStopped);
}

enzo::ui::DropdownPopup::DropdownPopup(Dropdown* owner)
    : QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint), owner_(owner)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void enzo::ui::DropdownPopup::openBeneath(QWidget* anchor, int selectedIndex)
{
    selectedIndex_ = selectedIndex;
    hoveredIndex_ = selectedIndex;
    scrollOffset_ = 0;
    closing_ = false;

    const int fullHeight = std::min(contentHeight(), maxPopupHeight);
    const QPoint topLeft = anchor->mapToGlobal(QPoint(0, anchor->height() + popupGap));
    setGeometry(topLeft.x(), topLeft.y(), anchor->width(), fullHeight);

    revealedHeight_ = 0;
    fadeElapsed_ = 0;
    show();
    setFocus();

    // Unroll the list to its full height
    auto reveal = new QPropertyAnimation(this, "revealedHeight", this);
    reveal->setDuration(180);
    reveal->setEasingCurve(QEasingCurve::OutCubic);
    reveal->setStartValue(0);
    reveal->setEndValue(fullHeight);
    reveal->start(QAbstractAnimation::DeleteWhenStopped);

    // Fade each row in turn from the top
    const qreal fadeTotal = itemStaggerMs * (owner_->items_.size() - 1) + itemFadeMs;
    auto fade = new QPropertyAnimation(this, "fadeElapsed", this);
    fade->setDuration(static_cast<int>(fadeTotal));
    fade->setStartValue(0.0);
    fade->setEndValue(fadeTotal);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

int enzo::ui::DropdownPopup::contentHeight() const
{
    return static_cast<int>(owner_->items_.size()) * itemHeight + padding * 2;
}

int enzo::ui::DropdownPopup::maxScrollOffset() const
{
    return std::max(0, contentHeight() - height());
}

int enzo::ui::DropdownPopup::rowAt(int localY) const
{
    if (localY < 0 || localY > revealedHeight_) return -1;
    const int index = (localY - padding + scrollOffset_) / itemHeight;
    if (index < 0 || index >= static_cast<int>(owner_->items_.size())) return -1;
    return index;
}

void enzo::ui::DropdownPopup::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Grow the rounded box with the reveal so the bottom edge stays clean
    QRectF box = QRectF(0, 0, width(), revealedHeight_).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath clip;
    clip.addRoundedRect(box, borderRadius, borderRadius);
    painter.setClipPath(clip);

    painter.fillRect(box, backgroundColor);

    // Rows, translated by the scroll offset and naturally clipped to the reveal
    for (int index = 0; index < static_cast<int>(owner_->items_.size()); ++index)
    {
        const int top = padding + index * itemHeight - scrollOffset_;
        if (top + itemHeight < 0 || top > revealedHeight_) continue;

        QRect row(0, top, width(), itemHeight);
        if (index == hoveredIndex_)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(hoverColor);
            painter.drawRoundedRect(row.adjusted(padding, 1, -padding, -1), 5, 5);
        }

        const qreal opacity =
            std::clamp((fadeElapsed_ - index * itemStaggerMs) / itemFadeMs, 0.0, 1.0);
        painter.setOpacity(opacity);
        painter.setPen(index == selectedIndex_ ? selectedTextColor : textColor);
        painter.drawText(
            row.adjusted(textPadding, 0, -textPadding, 0),
            Qt::AlignVCenter | Qt::AlignLeft,
            owner_->items_[index].text
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
        const float visibleFraction = static_cast<float>(height()) / contentHeight();
        const int trackHeight = revealedHeight_ - padding * 2;
        const int thumbHeight = std::max(20, static_cast<int>(trackHeight * visibleFraction));
        const float scrollFraction = static_cast<float>(scrollOffset_) / maxScrollOffset();
        const int thumbTop =
            padding + static_cast<int>((trackHeight - thumbHeight) * scrollFraction);
        painter.setPen(Qt::NoPen);
        painter.setBrush(borderColor);
        painter.drawRoundedRect(QRect(width() - 5, thumbTop, 3, thumbHeight), 1, 1);
    }

    // Outline drawn last so it sits above the fill
    painter.setClipping(false);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, borderRadius, borderRadius);
}

void enzo::ui::DropdownPopup::mouseMoveEvent(QMouseEvent* event)
{
    const int row = rowAt(event->pos().y());
    if (row != hoveredIndex_)
    {
        hoveredIndex_ = row;
        update();
    }
}

void enzo::ui::DropdownPopup::mousePressEvent(QMouseEvent* event)
{
    const QPoint pos = event->pos();
    const bool insideBox = QRect(0, 0, width(), revealedHeight_).contains(pos);
    if (insideBox)
    {
        const int row = rowAt(pos.y());
        if (row >= 0)
        {
            selectedIndex_ = row;
            Q_EMIT itemSelected(row);
        }
    }
    animateClose();
}

void enzo::ui::DropdownPopup::mouseReleaseEvent(QMouseEvent* event)
{
    // Press and hold to open, drag, then release over an item to select
    if (closing_) return;
    const int row = rowAt(event->pos().y());
    if (row < 0) return;

    selectedIndex_ = row;
    Q_EMIT itemSelected(row);
    animateClose();
}

void enzo::ui::DropdownPopup::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        animateClose();
        return;
    }
    QWidget::keyPressEvent(event);
}

void enzo::ui::DropdownPopup::wheelEvent(QWheelEvent* event)
{
    if (maxScrollOffset() == 0) return;
    scrollOffset_ = std::clamp(scrollOffset_ - event->angleDelta().y(), 0, maxScrollOffset());
    update();
}

void enzo::ui::DropdownPopup::animateClose()
{
    if (closing_) return;
    closing_ = true;
    Q_EMIT aboutToClose();

    auto collapse = new QPropertyAnimation(this, "revealedHeight", this);
    collapse->setDuration(140);
    collapse->setEasingCurve(QEasingCurve::InCubic);
    collapse->setStartValue(revealedHeight_);
    collapse->setEndValue(0);
    connect(collapse, &QPropertyAnimation::finished, this, [this] { hide(); });
    collapse->start(QAbstractAnimation::DeleteWhenStopped);
}

void enzo::ui::DropdownPopup::hideEvent(QHideEvent* event)
{
    closing_ = false;
    Q_EMIT closed();
    QWidget::hideEvent(event);
}
