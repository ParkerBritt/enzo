#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOperator.h"
#include <QLabel>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qwidget.h>
#include <QButtonGroup>
#include <QPainter>
#include <QPaintEvent>
#include <icecream.hpp>

GeoSheetModeButton::GeoSheetModeButton(QWidget* parent)
: QPushButton(parent)
{
    setFixedSize(QSize(23,23));
    setObjectName("GeoSheetModeButton");
    setCheckable(true);
    // setStyleSheet(
    // R"(
    //     #GeoSheetModeButton
    //     {
    //         background: transparent;
    //         border: none;
    //     }
    //     #GeoSheetModeButton::checked
    //     {
    //         opacity: 50;
    //         border: none;
    //     }
    // )");

}

void GeoSheetModeButton::enterEvent(QEnterEvent *event)
{
    hovered_=true;
    QPushButton::enterEvent(event);

}

void GeoSheetModeButton::leaveEvent(QEvent *event)
{
    hovered_=false;
    QPushButton::leaveEvent(event);
}

void GeoSheetModeButton::paintEvent(QPaintEvent* event)
{
    const QIcon buttonIcon = icon();

    if(!buttonIcon.isNull())
    {
        QPainter painter(this);

        QSize size = iconSize();

        if(!isChecked())
        {
            painter.setOpacity(0.5);
        }
        if(hovered_)
        {
            size*=1.1;
        }



        QPixmap pixmap = buttonIcon.pixmap(size);
        QPoint center = rect().center() - QPoint(size.width() / 2, size.height() / 2);
        painter.drawPixmap(center, pixmap);

    }

}


GeoSheetMenuBarModeSelection::GeoSheetMenuBarModeSelection(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    mainLayout_ = new QHBoxLayout();
    constexpr int mainMargin = 0;
    mainLayout_->setContentsMargins(mainMargin,mainMargin,mainMargin,mainMargin);

    QWidget* buttonBg = new QWidget();
    buttonBg->setObjectName("GeoSheetMenuBarButtonBg");
    buttonBg->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    constexpr int bgSizeMargin = 5;
    buttonBg->setContentsMargins(bgSizeMargin,0,bgSizeMargin,0);
    buttonBg->setStyleSheet(
    R"(
        #GeoSheetMenuBarButtonBg
        {
            background-color: #242424;
            border-radius: 8px;
        }

    )");
    QHBoxLayout* buttonBgLayout = new QHBoxLayout();
    constexpr int margin = 0;
    buttonBgLayout->setContentsMargins(margin,margin,margin,margin);

    modeButtonGroup_.setExclusive(true);

    auto newButton = [this, &buttonBgLayout](const char* tooltip="", const char* iconPath=":/icons/attributePoint.svg")
    {
        auto newButton = new GeoSheetModeButton();
        newButton->setToolTip(tooltip);
        newButton->setIcon(QIcon(iconPath));
        modeButtonGroup_.addButton(newButton);
        buttonBgLayout->addWidget(newButton);
        return newButton;
    };

    pointButton = newButton("Point Attributes", ":/icons/attributePoint.svg");
    vertexButton = newButton("Vertex Attributes", ":/icons/attributeVertex.svg");
    primitiveButton = newButton("Primitive Attributes", ":/icons/attributePrimitive.svg");
    globalButton = newButton("Global Attributes", ":/icons/attributeGlobal.svg");

    pointButton->setChecked(true);

    buttonBg->setLayout(buttonBgLayout);

    mainLayout_->addWidget(buttonBg);

    setLayout(mainLayout_);
}



GeometrySpreadsheetMenuBar::GeometrySpreadsheetMenuBar(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    mainLayout_ = new QHBoxLayout();
    nodeLabel_ = new QLabel();
    modeSelection = new GeoSheetMenuBarModeSelection();

    mainLayout_->addWidget(nodeLabel_);
    mainLayout_->addStretch();
    mainLayout_->addWidget(modeSelection);
    setProperty("class", "GeometrySpreadsheetMenuBar");
    setStyleSheet(
    R"(
        .GeometrySpreadsheetMenuBar,
        .GeometrySpreadsheetMenuBar *
        {
            background-color: #1B1B1B;
        }
    )");

    constexpr int margins = 5;
    mainLayout_->setContentsMargins(margins, margins, margins, margins);

    setLayout(mainLayout_);
}

void GeometrySpreadsheetMenuBar::setNode(enzo::nt::OpId opId)
{
    enzo::nt::GeometryOperator& geoOp = enzo::nt::nm().getGeoOperator(opId);
    nodeLabel_->setText("<b>Node: </b>" + QString::fromStdString(geoOp.getLabel()));
}
