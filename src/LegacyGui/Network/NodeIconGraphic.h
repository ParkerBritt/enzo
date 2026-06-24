#pragma once
#include <QGraphicsSvgItem>

class NodeIconGraphic : public QGraphicsSvgItem
{
  public:
    NodeIconGraphic(const QString& fileName, QGraphicsItem* parent = nullptr);

  protected:
    // void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget =
    // nullptr) override;
};
