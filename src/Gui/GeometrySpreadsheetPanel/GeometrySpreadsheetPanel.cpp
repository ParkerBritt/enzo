#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetPanel.h"
#include "Engine/Network/NetworkManager.h"
#include "Gui/GeometrySpreadsheetPanel/AttributeSpreadsheetModel.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"
#include "Gui/GeometrySpreadsheetPanel/PrimitiveTreeModel.h"
#include <QLabel>
#include <QPainterPath>
#include <QTableWidget>
#include <QTreeWidget>
#include <qframe.h>
#include <qpushbutton.h>
#include <qtablewidget.h>

GeometrySpreadsheetPanel::GeometrySpreadsheetPanel(QWidget *parent) : Panel(parent) {
    primView_ = new QTreeView(parent);
    attributeView_ = new QTreeView(parent);

    attributeView_->setRootIsDecorated(false);
    attributeView_->setAlternatingRowColors(true);
    attributeView_->setUniformRowHeights(true); // improves performance
    attributeView_->setStyleSheet(R"(
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
            border: none;
            border-bottom: 1px solid #303030;
        }
    )");
    attributeView_->setFrameStyle(QFrame::NoFrame);

    model_ = new AttributeSpreadsheetModel();
    attributeView_->setModel(model_);

    primModel_ = new PrimitiveTreeModel(this);
    primView_->setModel(primModel_);
    primView_->setStyleSheet(R"(
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

        QTreeView::item:selected
        {
            background-color: #414141;
        }
        QHeaderView::section {
            background-color: #1B1B1B;
            border: none;
            border-bottom: 1px solid #303030;
        }
    )");

    connect(primView_->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &GeometrySpreadsheetPanel::onPrimitiveSelected);

    menuBar_ = new GeometrySpreadsheetMenuBar();
    // connect buttons
    connect(menuBar_->modeSelection->pointButton, &QPushButton::clicked, this,
            [this]() { model_->setOwner(enzo::ga::AttributeOwner::POINT); });
    connect(menuBar_->modeSelection->vertexButton, &QPushButton::clicked, this,
            [this]() { model_->setOwner(enzo::ga::AttributeOwner::VERTEX); });
    connect(menuBar_->modeSelection->primitiveButton, &QPushButton::clicked, this,
            [this]() { model_->setOwner(enzo::ga::AttributeOwner::PRIMITIVE); });
    connect(menuBar_->modeSelection->globalButton, &QPushButton::clicked, this,
            [this]() { model_->setOwner(enzo::ga::AttributeOwner::GLOBAL); });
    // set default
    menuBar_->modeSelection->pointButton->click();

    contentSplitter_ = new QSplitter(Qt::Horizontal);
    contentSplitter_->addWidget(primView_);
    contentSplitter_->addWidget(attributeView_);
    contentSplitter_->setStretchFactor(0, 3);
    contentSplitter_->setStretchFactor(1, 7);

    mainLayout_ = new QVBoxLayout();
    mainLayout_->setSpacing(0);
    mainLayout_->addWidget(menuBar_, 0);
    mainLayout_->addWidget(contentSplitter_, 1);

    setLayout(mainLayout_);
}

void GeometrySpreadsheetPanel::clear() {
    model_->clear();
    primModel_->clear();
}

void GeometrySpreadsheetPanel::setNode(enzo::nt::OpId opId) { menuBar_->setNode(opId); }

void GeometrySpreadsheetPanel::packetChanged(enzo::NodePacket &packet) {
    currentPacket_ = &packet;
    if (packet.size() > 0) {
        primModel_->setPacket(packet);
        // select first primitive by default
        primView_->setCurrentIndex(primModel_->index(0, 0));
    } else {
        clear();
    }
}

void GeometrySpreadsheetPanel::onPrimitiveSelected(const QModelIndex &current,
                                                   const QModelIndex &previous) {
    Q_UNUSED(previous)
    if (!current.isValid() || !currentPacket_)
        return;

    int row = current.row();
    if (row >= 0 && row < static_cast<int>(currentPacket_->size())) {
        model_->primitiveChanged(currentPacket_->getPrimitive(row));
        attributeView_->update();
    }
}

// void GeometrySpreadsheetPanel::resizeEvent(QResizeEvent *event)
// {
//     QPainterPath path;
//     constexpr float radius = 10;
//     path.addRoundedRect(mainLayout_->contentsRect(), radius, radius);
//     QRegion region = QRegion(path.toFillPolygon().toPolygon());
//     this->setMask(region);
// }
//
