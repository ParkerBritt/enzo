#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QSplitter>
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
    void geometryChanged(enzo::geo::Primitive& geometry);
    void clear();
    void setNode(enzo::nt::OpId opId);
private:
    QVBoxLayout* mainLayout_;
    // QHBoxLayout* contentLayout_;
    QWidget* bgWidget_;
    GeometrySpreadsheetModel* model_;

    QSplitter* contentSplitter_;

    QTreeView* primView_;
    QTreeView* attributeView_;
    GeometrySpreadsheetMenuBar* menuBar_;


};
