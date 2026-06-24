#include "LegacyGui/UtilWidgets/Dropdown.h"
#include "LegacyGui/IconRegistry.h"
#include "LegacyGui/Style.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <algorithm>

namespace {

constexpr int textPadding = 10;
constexpr int arrowSize = 14;
constexpr int arrowMargin = 8;
constexpr int popupGap = 2;

} // namespace

enzo::ui::Dropdown::Dropdown(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);

    popup_ = new DropdownPopup(this);
    connect(popup_, &DropdownPopup::itemSelected, this, &Dropdown::setCurrentIndex);
    connect(popup_, &DropdownPopup::aboutToClose, this, [this] { animateArrow(0); });
    connect(popup_, &DropdownPopup::closed, this, [this] {
        popupOpen_ = false;
        animateArrow(0);
    });
}

void enzo::ui::Dropdown::addItem(const QString& text, const QString& data)
{
    items_.push_back({QIcon(), text, data});
    if (currentIndex_ < 0) currentIndex_ = 0;
    updateGeometry();
    update();
}

QString enzo::ui::Dropdown::currentText() const
{
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(items_.size())) return {};
    return items_[currentIndex_].text;
}

QString enzo::ui::Dropdown::currentData() const
{
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(items_.size())) return {};
    return items_[currentIndex_].data;
}

void enzo::ui::Dropdown::setCurrentIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(items_.size())) return;
    if (index == currentIndex_) return;
    currentIndex_ = index;
    update();
    Q_EMIT currentIndexChanged(index);
}

void enzo::ui::Dropdown::setCurrentData(const QString& data)
{
    for (int index = 0; index < static_cast<int>(items_.size()); ++index)
    {
        if (items_[index].data == data)
        {
            setCurrentIndex(index);
            return;
        }
    }
}

QSize enzo::ui::Dropdown::sizeHint() const
{
    int widest = 0;
    for (const Item& item : items_)
    {
        widest = std::max(widest, fontMetrics().horizontalAdvance(item.text));
    }
    const int width = textPadding * 2 + widest + arrowSize + arrowMargin;
    return {width, enzo::style::parameter::height};
}

void enzo::ui::Dropdown::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Box fill and border
    QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(enzo::style::dropdown::borderColor, 1));
    painter.setBrush(enzo::style::dropdown::backgroundColor);
    painter.drawRoundedRect(
        box,
        enzo::style::parameter::borderRadius,
        enzo::style::parameter::borderRadius
    );

    // Current selection text
    painter.setPen(enzo::style::dropdown::foregroundColor);
    QRect textRect = rect().adjusted(textPadding, 0, -(arrowSize + arrowMargin), 0);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, currentText());

    // Indicator chevron pulled from the icon registry, rotated while the popup is open
    const QPixmap chevron = IconRegistry::instance().pixmap(
        "chevron-down",
        QSize(arrowSize, arrowSize),
        enzo::style::dropdown::foregroundColor
    );
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

enzo::ui::DropdownPopup::DropdownPopup(Dropdown* owner) : PopupList(nullptr), owner_(owner) {}

void enzo::ui::DropdownPopup::openBeneath(QWidget* anchor, int selectedIndex)
{
    items_ = owner_->items_;
    showAllItems();
    const QPoint topLeft = anchor->mapToGlobal(QPoint(0, anchor->height() + popupGap));
    openList(topLeft, anchor->width(), selectedIndex);
}
