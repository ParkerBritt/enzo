#include "Gui/Interface.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/Geometry.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetPanel.h"
#include "Gui/ParametersPanel/ParametersPanel.h"
#include "Gui/Viewport/Viewport.h"
#include "Gui/Network/Network.h"
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qsplitter.h>
#include <QTimer>
#include <Gui/UtilWidgets/Splitter.h>
#include <icecream.hpp>

EnzoUI::EnzoUI()
{
    // layout
    mainLayout_ = new QVBoxLayout(this);
    setLayout(mainLayout_);

    // title and size
    setWindowTitle("Enzo");
    constexpr float scale = 0.8;
    resize(1920*scale, 1080*scale);

    // TODO: unify stylsheet
    setStyleSheet("background-color:#1d2021;");

    Viewport* viewport = new Viewport();
    Network* network = new Network();
    ParametersPanel* parametersPanel = new ParametersPanel();
    GeometrySpreadsheetPanel* geometrySpreadsheetPanel = new GeometrySpreadsheetPanel();

    constexpr int margin = 2;
    viewport->layout()->setContentsMargins(margin, margin, margin, margin);
    network->layout()->setContentsMargins(margin, margin, margin, margin);
    parametersPanel->layout()->setContentsMargins(margin, margin, margin, margin);
    geometrySpreadsheetPanel->layout()->setContentsMargins(margin, margin, margin, margin);
    mainLayout_->setContentsMargins(margin, margin, margin, margin);



    // TODO: dynamic splitters
    viewportSplitter_ = new Splitter(this);
    networkSplitter_ = new Splitter(this);
    spreadsheetSplitter_ = new Splitter(this);
    networkSplitter_->setOrientation(Qt::Vertical);
    spreadsheetSplitter_->setOrientation(Qt::Vertical);

    spreadsheetSplitter_->addWidget(viewport);
    spreadsheetSplitter_->addWidget(geometrySpreadsheetPanel);
    spreadsheetSplitter_->setSizes({200,100});

    viewportSplitter_->addWidget(spreadsheetSplitter_);
    viewportSplitter_->addWidget(networkSplitter_);
    viewportSplitter_->setSizes({100,200});

    networkSplitter_->addWidget(parametersPanel);
    networkSplitter_->addWidget(network);
    networkSplitter_->setSizes({40,100});

    mainLayout_->addWidget(viewportSplitter_);

    // connect signals
    enzo::nt::nm().selectedNodesChanged.connect([parametersPanel](std::vector<enzo::nt::OpId> selectedNodeIds){
        if(selectedNodeIds.size()<=0) return;
        parametersPanel->selectionChanged(selectedNodeIds.back());
    });
    enzo::nt::nm().selectedNodesChanged.connect([geometrySpreadsheetPanel](std::vector<enzo::nt::OpId> selectedNodeIds){
        if(selectedNodeIds.size()<=0) return;
        geometrySpreadsheetPanel->setNode(selectedNodeIds.back());
    });
    enzo::nt::nm().selectedNodesChanged.connect([geometrySpreadsheetPanel](std::vector<enzo::nt::OpId> selectedNodeIds){
        if(selectedNodeIds.size()<=0) return;
        auto& geometry = enzo::nt::nm().getGeoOperator(selectedNodeIds.back()).getOutputGeo(0);
        geometrySpreadsheetPanel->geometryChanged(geometry);
    });
    enzo::nt::nm().displayGeoChanged.connect([viewport](enzo::geo::Geometry& geometry){viewport->setGeometry(geometry);});
}
