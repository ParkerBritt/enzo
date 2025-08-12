#pragma once
#include <QGraphicsItem>
#include <QPainter>
#include "Engine/Types.h"
#include "Gui/Network/DisplayFlagButton.h"
#include "Gui/Network/SocketGraphic.h"
#include "Gui/Network/NodeIconGraphic.h"
#include "Gui/Network/NodeEdgeGraphic.h"
#include <iostream>

class NodeGraphic
: public QGraphicsItem
{
public:
    NodeGraphic(enzo::nt::OpId id, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    
    SocketGraphic* getInput(int indx) const;
    SocketGraphic* getOutput(int indx) const;
    enzo::nt::OpId getOpId() const;
    QPointF getSocketPosition(int socketIndex, enzo::nt::SocketIOType socketType);
    QPointF getSocketScenePosition(int socketIndex, enzo::nt::SocketIOType socketType);
    QRectF getBodyRect();
    void setDisplayFlag(bool state);
    void setSelected(bool selected);
    bool toggleSelected();


    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;




private:
    void initSockets();
    void initFlagButtons();
    void initIcon();
    void initFonts();

    enzo::nt::OpId opId_;
    std::vector<SocketGraphic*> inputs_;
    std::vector<SocketGraphic*> outputs_;

    // std::vector<NodeEdgeGraphic*> edges_;
    bool selected_=false;
    std::string titleText_="";
    std::string subTitleText_="";
    int maxTitleLen_=10;
    QRectF bodyRect_;
    int socketSize_ = 1;
    int inputSocketCnt_=0;
    int outputSocketCnt_=0;
    NodeIconGraphic* icon_;
    int iconScale_;
    float iconPadding_ = 5;
    QGraphicsItem* test_;

    QFont titleFont_;
    QFont subTitleFont_;
    int titlePadding_;

    QRectF titleRect_;
    QRectF subTitleRect_;

    DisplayFlagButton* displayFlagButton_;

    void updatePositions();
protected:
        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;
        // void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        // void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

};

