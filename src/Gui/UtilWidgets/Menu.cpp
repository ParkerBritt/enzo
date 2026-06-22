#include "Gui/UtilWidgets/Menu.h"
#include "Gui/Style.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace {

// Mirrors the row padding inside PopupList so a child aligns to its parent row
constexpr int listPadding = 4;

// A child overlaps the parent border slightly so the seam between them reads as one
constexpr int submenuOverlap = 4;

// Branch chevron drawn at the right edge of a row
constexpr int chevronReach = 4;
constexpr int chevronInset = 10;

// Rows get extra room beyond the text and never fall below a floor
constexpr int textBreathingRoom = 24;
constexpr int minMenuWidth = 168;

} // namespace

enzo::ui::Menu::Menu(QWidget* parent) : PopupList(parent)
{
    // Menu rows show at full opacity right away
    setRowFadeEnabled(false);

    // Closing a level folds its open child away with it
    connect(this, &PopupList::aboutToClose, this, [this] { closeSubmenu(); });
}

enzo::ui::Menu::~Menu() { delete submenu_; }

void enzo::ui::Menu::setEntries(std::vector<Entry> entries)
{
    entries_ = std::move(entries);
    items_.clear();
    for (const Entry& entry : entries_)
    {
        items_.push_back({entry.icon, entry.text, {}});
    }
    showAllItems();
}

void enzo::ui::Menu::popup(const QPoint& globalTopLeft, bool takeFocus)
{
    // A reopen while visible folds any submenu away before the move
    closeSubmenu();
    openList(globalTopLeft, menuWidth(), 0, takeFocus);
}

const enzo::ui::Menu::Entry& enzo::ui::Menu::entryAt(int position) const
{
    return entries_[visibleIndices_[position]];
}

bool enzo::ui::Menu::isBranch(int position) const
{
    if (position < 0 || position >= static_cast<int>(visibleIndices_.size())) return false;
    return !entryAt(position).children.empty();
}

void enzo::ui::Menu::onRowActivated(int position)
{
    if (isBranch(position))
    {
        openSubmenu(position, true);
        return;
    }

    const Entry& entry = entryAt(position);
    if (entry.action) entry.action();
    animateClose();
}

void enzo::ui::Menu::onHighlightChanged(int position, bool fromPointer)
{
    // Hovering a branch reveals its child. Keyboard travel waits for the Right key
    // and only folds away whatever child is open.
    if (fromPointer && isBranch(position))
        openSubmenu(position, false);
    else
        closeSubmenu();
}

void enzo::ui::Menu::openSubmenu(int position, bool takeFocus)
{
    if (!isBranch(position)) return;
    if (submenuPosition_ == position && submenu_ && submenu_->isVisible()) return;

    closeSubmenu();
    submenuPosition_ = position;

    if (!submenu_)
    {
        // Parented so the child shares this menu's popup stack
        submenu_ = new Menu(this);
        submenu_->parentMenu_ = this;
        connect(submenu_, &PopupList::closed, this, &Menu::onSubmenuClosed);
    }

    submenu_->setEntries(entryAt(position).children);

    // Anchor the child to the right edge of the row with its first row aligned
    const QRect row = rowRect(position);
    const QPoint anchor = mapToGlobal(QPoint(width() - submenuOverlap, row.top() - listPadding));
    submenu_->popup(anchor, takeFocus);
}

void enzo::ui::Menu::mouseMoveEvent(QMouseEvent* event)
{
    routeHover(event->globalPosition().toPoint());
}

void enzo::ui::Menu::mousePressEvent(QMouseEvent* event)
{
    // A press over the owning widget is handed to it so a menu bar can toggle
    // or switch menus. Any other press keeps the base behavior.
    const QPoint globalPos = event->globalPosition().toPoint();
    if (QWidget* owner = ownerAt(globalPos))
    {
        QMouseEvent forwarded(
            event->type(),
            owner->mapFromGlobal(globalPos),
            globalPos,
            event->button(),
            event->buttons(),
            event->modifiers()
        );
        QCoreApplication::sendEvent(owner, &forwarded);
        return;
    }
    PopupList::mousePressEvent(event);
}

QWidget* enzo::ui::Menu::ownerAt(const QPoint& globalPos) const
{
    const Menu* root = this;
    while (root->parentMenu_)
        root = root->parentMenu_;
    QWidget* owner = root->parentWidget();
    if (!owner) return nullptr;
    if (!owner->rect().contains(owner->mapFromGlobal(globalPos))) return nullptr;
    return owner;
}

void enzo::ui::Menu::dismiss() { animateClose(); }

void enzo::ui::Menu::routeHover(const QPoint& globalPos)
{
    // The deepest open menu receives the move. Hand it up to whichever level the
    // cursor sits over so focus and highlight track the pointer.
    Menu* target = this;
    while (target && !target->frameGeometry().contains(globalPos))
    {
        target = target->parentMenu_;
    }

    // A miss on every level is handed to the widget owning the root menu,
    // mirroring how QMenu cooperates with QMenuBar
    if (!target)
    {
        QWidget* owner = ownerAt(globalPos);
        if (!owner) return;
        QMouseEvent forwarded(
            QEvent::MouseMove,
            owner->mapFromGlobal(globalPos),
            globalPos,
            Qt::NoButton,
            Qt::NoButton,
            Qt::NoModifier
        );
        QCoreApplication::sendEvent(owner, &forwarded);
        return;
    }

    if (!target->hasFocus()) target->setFocus();
    target->hoverRowAt(target->mapFromGlobal(globalPos).y());
}

void enzo::ui::Menu::closeSubmenu()
{
    submenuPosition_ = -1;
    if (!submenu_) return;

    // Fold the deepest level first so no descendant is left orphaned
    submenu_->closeSubmenu();
    if (submenu_->isVisible())
    {
        suppressCascade_ = true;
        submenu_->hide();
    }
}

void enzo::ui::Menu::onSubmenuClosed()
{
    submenuPosition_ = -1;
    if (suppressCascade_)
    {
        suppressCascade_ = false;
        return;
    }

    // The child closed on its own through a selection, Escape, or an outside click,
    // so this level folds away too and the close travels up to the root
    animateClose();
}

void enzo::ui::Menu::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Right:
        if (isBranch(highlightedPosition()))
        {
            openSubmenu(highlightedPosition(), true);
            submenu_->setFocus();
            submenu_->setHighlightedPosition(0);
        }
        return;
    case Qt::Key_Left:
        if (parentMenu_)
        {
            parentMenu_->setFocus();
            parentMenu_->closeSubmenu();
        }
        return;
    default:
        PopupList::keyPressEvent(event);
    }
}

void enzo::ui::Menu::paintRowDecoration(QPainter& painter, int position, const QRect& row)
{
    if (!isBranch(position)) return;

    const qreal tipX = row.right() - chevronInset;
    const qreal midY = row.center().y() + 0.5;
    QPainterPath chevron;
    chevron.moveTo(tipX - chevronReach, midY - chevronReach);
    chevron.lineTo(tipX, midY);
    chevron.lineTo(tipX - chevronReach, midY + chevronReach);

    QPen pen(enzo::style::menu::chevronColor, 1.2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(chevron);
}

int enzo::ui::Menu::menuWidth() const
{
    int width = preferredWidth() + textBreathingRoom;
    for (const Entry& entry : entries_)
    {
        if (!entry.children.empty())
        {
            width += chevronInset;
            break;
        }
    }
    return std::max(width, minMenuWidth);
}
