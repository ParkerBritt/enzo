#include "LegacyGui/Network/TabMenu.h"
#include "Engine/Network/OperatorTable.h"
#include "LegacyGui/Network/NetworkPanel.h"

#include <QApplication>
#include <QCursor>
#include <QIcon>
#include <QKeyEvent>
#include <QResizeEvent>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

constexpr int menuWidth = 280;
constexpr int searchMargin = 4;
constexpr int searchHeight = 28;

} // namespace

enzo::ui::TabMenu::TabMenu(NetworkPanel* network) : PopupList(network), network_(network)
{
    searchBar_ = new QLineEdit(this);
    searchBar_->setFocusPolicy(Qt::NoFocus);
    searchBar_->setFrame(false);
    searchBar_->setAlignment(Qt::AlignCenter);
    searchBar_->setObjectName("TabMenuSearch");
    searchBar_->setStyleSheet(
        QString(R"(
    QWidget#TabMenuSearch {
        background-color: %1;
        padding: 3px;
        border-radius: 6px;
        border: 1px solid %2;
    }
    )")
            .arg(enzo::style::color::surfaceDeep.name(), enzo::style::color::border.name())
    );
    connect(searchBar_, &QLineEdit::textChanged, this, &TabMenu::applyFilter);

    // Build one row per operator from the table
    for (const op::OpInfo& tableItem : op::OperatorTable::getData())
    {
        Item item;
        item.icon = QIcon(":/node-icons/grid.svg");
        item.text = tableItem.displayName.c_str();
        item.data = tableItem.internalName.c_str();
        addItem(item);
    }

    connect(this, &PopupList::itemSelected, this, [this](int index) {
        createNode(items_[index].data.toStdString());
    });
}

int enzo::ui::TabMenu::headerHeight() const { return searchMargin * 2 + searchHeight; }

void enzo::ui::TabMenu::showOnMouse(float dx, float dy)
{
    searchBar_->clear();
    applyFilter("");
    const QPoint cursor = QCursor::pos() + QPoint(dx, dy);
    openList(cursor, menuWidth, 0);
}

void enzo::ui::TabMenu::applyFilter(const QString& text)
{
    const QString needle = text.toLower();
    visibleIndices_.clear();
    for (int index = 0; index < static_cast<int>(items_.size()); ++index)
    {
        if (needle.isEmpty() || items_[index].text.toLower().contains(needle))
        {
            visibleIndices_.push_back(index);
        }
    }

    // Refit only while open so the first open animates through openList instead
    if (isVisible())
    {
        fitToContents(true);
        setHighlightedPosition(0);
    }
}

void enzo::ui::TabMenu::createNode(const std::string& nodeName)
{
    std::optional<op::OpInfo> opInfo = op::OperatorTable::getOpInfo(nodeName);
    if (!opInfo.has_value())
    {
        throw std::runtime_error("Couldn't find op info for: " + nodeName);
    }
    network_->createNode(opInfo.value());
}

void enzo::ui::TabMenu::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Tab:
        animateClose();
        return;
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Escape:
        PopupList::keyPressEvent(event);
        return;
    default:
        // Typing and editing keys drive the search field
        QApplication::sendEvent(searchBar_, event);
    }
}

void enzo::ui::TabMenu::resizeEvent(QResizeEvent* event)
{
    PopupList::resizeEvent(event);
    searchBar_->setGeometry(searchMargin, searchMargin, width() - searchMargin * 2, searchHeight);
}
