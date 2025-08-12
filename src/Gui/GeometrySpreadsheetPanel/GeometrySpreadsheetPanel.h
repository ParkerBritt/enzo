#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <qtreeview.h>
#include "Engine/Types.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetModel.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"

class GeometrySpreadsheetPanel
: public QWidget
{
public:
    GeometrySpreadsheetPanel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
public Q_SLOTS:
    void geometryChanged(enzo::geo::Geometry& geometry);
    void setNode(enzo::nt::OpId opId);
private:
    QVBoxLayout* mainLayout_;
    QWidget* bgWidget_;
    GeometrySpreadsheetModel* model_;
    QTreeView* view_;
    GeometrySpreadsheetMenuBar* menuBar_;
protected:
    void resizeEvent(QResizeEvent *event) override;


};
