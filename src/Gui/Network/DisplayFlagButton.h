#pragma once
#include <QBrush>
#include <QGraphicsItem>

class DisplayFlagButton : public QGraphicsItem
{
  public:
    DisplayFlagButton(QGraphicsItem* parent = nullptr);
    float getWidth();
    void setEnabled(bool enabled);

  private:
    QRectF baseRect_;
    QColor disabledColor_ = QColor("#373737");
    QColor enabledColor_ = QColor("#00BFFF");
    QColor hoveredColor_ = QColor("#666666");
    QBrush disabledBrush_;
    QBrush enabledBrush_;
    QBrush hoveredDisabledBrush_;
    QBrush hoveredEnabledBrush_;
    bool hovered_ = false;
    bool enabled_ = false;

  protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
};
