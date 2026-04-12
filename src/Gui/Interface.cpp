#include "Gui/Interface.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/NodePacket.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetPanel.h"
#include "Gui/HeaderBar/HeaderBar.h"
#include "Gui/Network/NetworkPanel.h"
#include "Gui/Viewport/Viewport.h"
#include <Gui/UtilWidgets/Splitter.h>
#include <QTimer>
#include <icecream.hpp>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qsplitter.h>

EnzoUI::EnzoUI() {
    // layout
    mainLayout_ = new QVBoxLayout(this);
    setLayout(mainLayout_);

    // title and size
    setWindowTitle("Enzo");
    constexpr float scale = 0.8;
    resize(1920 * scale, 1080 * scale);

    // TODO: unify stylsheet
    setStyleSheet(R"(
    QWidget
    {
        background-color: #1a1a1a; 
        color: rgba(255,255,255,0.8);
    }
    QSplitter::handle
    {
        image: none;
    }
    )");

    viewport_ = new Viewport();
    network_ = new NetworkPanel();
    parametersPanel_ = new ParametersPanel();
    geometrySpreadsheetPanel_ = new GeometrySpreadsheetPanel();

    constexpr int margin = 2;
    viewport_->layout()->setContentsMargins(margin, margin, margin, margin);
    network_->layout()->setContentsMargins(margin, margin, margin, margin);
    parametersPanel_->layout()->setContentsMargins(margin, margin, margin, margin);
    geometrySpreadsheetPanel_->layout()->setContentsMargins(margin, margin, margin, margin);
    mainLayout_->setContentsMargins(margin, margin, margin, margin);

    // TODO: dynamic splitters
    viewportSplitter_ = new Splitter(this);
    networkSplitter_ = new Splitter(this);
    spreadsheetSplitter_ = new Splitter(this);
    networkSplitter_->setOrientation(Qt::Vertical);
    spreadsheetSplitter_->setOrientation(Qt::Vertical);

    spreadsheetSplitter_->addWidget(viewport_);
    spreadsheetSplitter_->addWidget(geometrySpreadsheetPanel_);
    spreadsheetSplitter_->setSizes({200, 100});

    viewportSplitter_->addWidget(spreadsheetSplitter_);
    viewportSplitter_->addWidget(networkSplitter_);
    viewportSplitter_->setSizes({100, 200});

    networkSplitter_->addWidget(parametersPanel_);
    networkSplitter_->addWidget(network_);
    networkSplitter_->setSizes({40, 100});

    HeaderBar *header = new HeaderBar();

    mainLayout_->addWidget(header);
    mainLayout_->addWidget(viewportSplitter_);

    connectSignals();
}

void EnzoUI::connectSignals() {

    // Selection changed
    enzo::nt::nm().selectedNodesChanged.connect(
        [this](std::vector<enzo::nt::OpId> selectedNodeIds) {
            if (selectedNodeIds.empty()) {
                parametersPanel_->clearParameters();
                geometrySpreadsheetPanel_->clear();
                return;
            }
            enzo::nt::OpId selectedId = selectedNodeIds.back();
            parametersPanel_->selectionChanged(selectedId);
            geometrySpreadsheetPanel_->setNode(selectedId);
            auto packet = enzo::nt::nm().getGeoOperator(selectedId).getOutputPacket(0);
            geometrySpreadsheetPanel_->packetChanged(packet);
        });

    // Operator created
    enzo::nt::nm().operatorCreated.connect(
        [this](enzo::nt::OpId opId) { network_->onOperatorCreated(opId); });

    // Connection created
    enzo::nt::nm().connectionCreated.connect(
        [this](std::weak_ptr<enzo::nt::GeometryConnection> conn) {
            network_->onConnectionCreated(conn);
        });

    // Display/geometry changed
    enzo::nt::nm().displayGeoChanged.connect(
        [this](std::shared_ptr<const enzo::NodePacket> packet) { viewport_->setGeometry(packet); });
    enzo::nt::nm().selectedGeoChanged.connect(
        [this](std::shared_ptr<const enzo::NodePacket> packet) { geometrySpreadsheetPanel_->packetChanged(packet); });

    // Network cleared
    enzo::nt::nm().networkCleared.connect([this]() {
        network_->clearNetwork();
        viewport_->clearGeometry();
        geometrySpreadsheetPanel_->clear();
    });
}
