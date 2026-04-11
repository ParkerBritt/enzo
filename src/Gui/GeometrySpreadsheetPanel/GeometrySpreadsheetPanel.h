#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <memory>
#include <qtreeview.h>
#include "Engine/Types.h"
#include "Engine/Operator/NodePacket.h"
#include "Gui/GeometrySpreadsheetPanel/AttributeSpreadsheetModel.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"
#include "Gui/GeometrySpreadsheetPanel/PrimitiveTreeModel.h"
#include "Gui/Panels/Panel.h"

class GeometrySpreadsheetPanel
: public Panel
{
public:
    GeometrySpreadsheetPanel(QWidget *parent = nullptr);
public Q_SLOTS:
    void packetChanged(std::shared_ptr<const enzo::NodePacket> packet);
    void clear();
    void setNode(enzo::nt::OpId opId);
private:
    QVBoxLayout* mainLayout_;
    // QHBoxLayout* contentLayout_;
    QWidget* bgWidget_;
    void onPrimitiveSelected(const QModelIndex &current, const QModelIndex &previous);

    AttributeSpreadsheetModel* model_;
    PrimitiveTreeModel* primModel_;
    std::shared_ptr<const enzo::NodePacket> currentPacket_;

    QSplitter* contentSplitter_;

    QTreeView* primView_;
    QTreeView* attributeView_;
    GeometrySpreadsheetMenuBar* menuBar_;


};
