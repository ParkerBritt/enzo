#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetPanel.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetModel.h"
#include "Engine/Network/NetworkManager.h"
#include <QTableWidget>
#include <QTreeWidget>
#include <QLabel>
#include <qframe.h>
#include <qpushbutton.h>
#include <qtablewidget.h>
#include <QPainterPath>

GeometrySpreadsheetPanel::GeometrySpreadsheetPanel(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
    mainLayout_ = new QVBoxLayout();
    mainLayout_->setSpacing(0);


    view_ = new QTreeView(parent);
    view_->setRootIsDecorated(false);
    view_->setAlternatingRowColors(true);
    view_->setUniformRowHeights(true); // improves performance
    view_->setStyleSheet(R"(
        QTreeView {
            background-color: #282828;
            alternate-background-color: #242424;
            paint-alternating-row-colors-for-empty-area: 1;
        }
        QTreeView QScrollBar {
            background: #1B1B1B;
            width: 15px;
        }
        QTreeView QScrollBar::handle:vertical {
            background: #282828;
            min-height: 50px;
            border-radius: 5px;
            border-width: 1px;
            border-color: #2D2D2D;
            border-style: solid;
            margin:2px;
        }

        QTreeView QScrollBar::add-page:vertical,
        QTreeView QScrollBar::sub-page:vertical,
        QTreeView QScrollBar::add-line:vertical,
        QTreeView QScrollBar::sub-line:vertical
        { height: 0px; }

        QHeaderView::section {
            background-color: #1B1B1B;
        }
    )");
    view_->setFrameStyle(QFrame::NoFrame);
    
    model_ = new GeometrySpreadsheetModel();
    view_->setModel(model_);

    menuBar_ = new GeometrySpreadsheetMenuBar();
    // connect buttons
    connect(menuBar_->modeSelection->pointButton, &QPushButton::clicked, this, [this](){model_->setOwner(enzo::ga::AttributeOwner::POINT);});
    connect(menuBar_->modeSelection->vertexButton, &QPushButton::clicked, this, [this](){model_->setOwner(enzo::ga::AttributeOwner::VERTEX);});
    connect(menuBar_->modeSelection->primitiveButton, &QPushButton::clicked, this, [this](){model_->setOwner(enzo::ga::AttributeOwner::PRIMITIVE);});
    connect(menuBar_->modeSelection->globalButton, &QPushButton::clicked, this, [this](){model_->setOwner(enzo::ga::AttributeOwner::GLOBAL);});
    // set default
    menuBar_->modeSelection->pointButton->click();


    mainLayout_->addWidget(menuBar_);
    mainLayout_->addWidget(view_);

    setLayout(mainLayout_);
}

void GeometrySpreadsheetPanel::setNode(enzo::nt::OpId opId)
{
    menuBar_->setNode(opId);
}


void GeometrySpreadsheetPanel::geometryChanged(enzo::geo::Geometry& geometry)
{
    model_->geometryChanged(geometry);
    view_->update();
}

void GeometrySpreadsheetPanel::resizeEvent(QResizeEvent *event)
{
    QPainterPath path;
    constexpr float radius = 10;
    path.addRoundedRect(mainLayout_->contentsRect(), radius, radius);
    QRegion region = QRegion(path.toFillPolygon().toPolygon());
    this->setMask(region);
}

