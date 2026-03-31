#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <qtreeview.h>
#include "Engine/Types.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetModel.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"
#include "Gui/Panels/Panel.h"

class GeometrySpreadsheetPanel
: public Panel
{
public:
    GeometrySpreadsheetPanel(QWidget *parent = nullptr);
public Q_SLOTS:
    void geometryChanged(enzo::geo::Geometry& geometry);
    void clear();
    void setNode(enzo::nt::OpId opId);
private:
    QVBoxLayout* mainLayout_;
    QWidget* bgWidget_;
    GeometrySpreadsheetModel* model_;
    QTreeView* view_;
    GeometrySpreadsheetMenuBar* menuBar_;


};
