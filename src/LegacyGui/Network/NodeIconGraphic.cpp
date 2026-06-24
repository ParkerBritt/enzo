#include "LegacyGui/Network/NodeIconGraphic.h"
#include <QBrush>
#include <QPainter>
#include <qgraphicssvgitem.h>

NodeIconGraphic::NodeIconGraphic(const QString& fileName, QGraphicsItem* parent)
    : QGraphicsSvgItem(fileName, parent)
{
}

// void NodeIconGraphic::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget
// *widget)
// {
//
//     this->setProperty("color", QColor("red"));
//     QGraphicsSvgItem::paint(painter, option, widget);
// }
