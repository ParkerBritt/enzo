#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Network/NodePacket.h"
#include "Gui/GeometrySpreadsheetPanel/AttributeSpreadsheetModel.h"
#include "Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.h"
#include "Gui/GeometrySpreadsheetPanel/PrimitiveTreeModel.h"
#include "Gui/Panels/Panel.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <qtreeview.h>

class GeometrySpreadsheetPanel : public Panel
{
  public:
    GeometrySpreadsheetPanel(QWidget* parent = nullptr);
  public Q_SLOTS:
    void packetChanged(std::shared_ptr<const enzo::NodePacket> packet);
    void clear();
    void setNode(enzo::nt::OpId opId);

  private:
    QVBoxLayout* mainLayout_;
    // QHBoxLayout* contentLayout_;
    QWidget* bgWidget_;
    void onPrimitiveSelected(const QModelIndex& current, const QModelIndex& previous);

    AttributeSpreadsheetModel* model_;
    PrimitiveTreeModel* primModel_;
    std::shared_ptr<const enzo::NodePacket> currentPacket_;

    QSplitter* contentSplitter_;

    QTreeView* primView_;
    QTreeView* attributeView_;
    GeometrySpreadsheetMenuBar* menuBar_;
};
