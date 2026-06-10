#include "Gui/UtilWidgets/MenuBar.h"

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>

namespace {

// Room around a title's text and the gap between neighbouring titles
constexpr int titlePaddingX = 8;
constexpr int titlePaddingY = 2;
constexpr int titleSpacing = 2;
constexpr int titleRadius = 6;

const QColor textColor("#B3B3B3");
const QColor highlightColor("#282828");

} // namespace

enzo::ui::MenuBar::MenuBar(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);

    // Parented here so the open menu hands events over the bar back to it
    menu_ = new Menu(this);
    connect(menu_, &PopupList::closed, this, [this] {
        openIndex_ = -1;
        hoverIndex_ = titleAt(mapFromGlobal(QCursor::pos()));
        update();
    });
}

void enzo::ui::MenuBar::addMenu(
    const QString& title,
    std::function<std::vector<Menu::Entry>()> build
)
{
    items_.push_back({title, std::move(build)});
    updateGeometry();
    update();
}

QSize enzo::ui::MenuBar::sizeHint() const
{
    int width = 0;
    for (int index = 0; index < static_cast<int>(items_.size()); ++index)
    {
        if (index > 0) width += titleSpacing;
        width += titleRect(index).width();
    }
    return QSize(width, fontMetrics().height() + titlePaddingY * 2);
}

QRect enzo::ui::MenuBar::titleRect(int index) const
{
    int left = 0;
    for (int before = 0; before < index; ++before)
    {
        left += fontMetrics().horizontalAdvance(items_[before].title) + titlePaddingX * 2 +
                titleSpacing;
    }
    const int width = fontMetrics().horizontalAdvance(items_[index].title) + titlePaddingX * 2;
    return QRect(left, 0, width, height());
}

int enzo::ui::MenuBar::titleAt(const QPoint& pos) const
{
    for (int index = 0; index < static_cast<int>(items_.size()); ++index)
    {
        if (titleRect(index).contains(pos)) return index;
    }
    return -1;
}

void enzo::ui::MenuBar::openMenuAt(int index)
{
    // A title with nothing to show leaves any open menu in place
    std::vector<Menu::Entry> entries = items_[index].build();
    if (entries.empty()) return;

    openIndex_ = index;
    menu_->setEntries(std::move(entries));
    const QRect title = titleRect(index);
    menu_->popup(mapToGlobal(QPoint(title.left(), title.bottom() + 1)));
    update();
}

void enzo::ui::MenuBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // While a menu is open its title carries the highlight, otherwise hover does
    const int litIndex = openIndex_ >= 0 ? openIndex_ : hoverIndex_;

    for (int index = 0; index < static_cast<int>(items_.size()); ++index)
    {
        const QRect title = titleRect(index);
        if (index == litIndex)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(highlightColor);
            painter.drawRoundedRect(title, titleRadius, titleRadius);
        }
        painter.setPen(textColor);
        painter.drawText(title, Qt::AlignCenter, items_[index].title);
    }
}

void enzo::ui::MenuBar::mouseMoveEvent(QMouseEvent* event)
{
    const int index = titleAt(event->pos());
    if (index != hoverIndex_)
    {
        hoverIndex_ = index;
        update();
    }

    // An open bar follows the cursor across titles like a native menu bar
    if (openIndex_ >= 0 && index >= 0 && index != openIndex_) openMenuAt(index);
}

void enzo::ui::MenuBar::mousePressEvent(QMouseEvent* event)
{
    const int index = titleAt(event->pos());

    // The open title toggles closed, another title switches, the gaps dismiss
    if (index >= 0 && index == openIndex_)
    {
        menu_->dismiss();
        return;
    }
    if (index >= 0)
    {
        openMenuAt(index);
        return;
    }
    if (openIndex_ >= 0) menu_->dismiss();
}

void enzo::ui::MenuBar::leaveEvent(QEvent*)
{
    hoverIndex_ = -1;
    update();
}
