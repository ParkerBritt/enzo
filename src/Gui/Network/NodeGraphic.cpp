#include "Gui/Network/NodeGraphic.h"
#include <QTextDocument>
#include <algorithm>
#include <iostream>
#include <qgraphicsitem.h>
#include <qnamespace.h>
#include <stdexcept>
#include <string>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Types.h"
#include "Gui/Network/DisplayFlagButton.h"
#include "Gui/Network/SocketGraphic.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSvgItem>
#include "Gui/Network/NodeIconGraphic.h"

NodeGraphic::NodeGraphic(enzo::nt::OpId id, QGraphicsItem *parent)
: QGraphicsItem(parent), opId_{id}
{
    socketSize_ = 3;
    titlePadding_=1;
    enzo::nt::GeometryOperator& geoOp = enzo::nt::nm().getGeoOperator(id);
    titleText_ = geoOp.getTypeName();
    // TODO: unique node names
    subTitleText_ = "OpID: " + std::to_string(opId_);
    constexpr int height = 27;
    constexpr int width = 100;
    bodyRect_ = QRect(-width*0.5f, -height*0.5f, width, height);
    iconScale_=height*0.55;

    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    initFonts();
    initIcon();
    initSockets();
    initFlagButtons();

}

void NodeGraphic::initFonts()
{
    // declare fonts
    titleFont_ = QFont();
    titleFont_.setPixelSize(8);

    subTitleFont_ = QFont();
    subTitleFont_.setPixelSize(5);

    // compute text positions
    QRectF titleBounds = QFontMetricsF(titleFont_).boundingRect(titleText_.c_str());
    QRectF subTitleBounds = QFontMetricsF(subTitleFont_).boundingRect(subTitleText_.c_str());
    float titleOffsetX = (titleBounds.width()-bodyRect_.width()-iconScale_)*0.5f;
    float titleOffsetY = subTitleBounds.height()*0.5f+titlePadding_+(-(subTitleBounds.height()*0.5+titleBounds.height()+titlePadding_)+bodyRect_.height())*0.5f;
    // float titleOffsetY = (bodyRect_.height())*0.5f;
    // titleOffsetY -= subTitleBounds.height()*0.5f;
    float subTitleOffsetX = titleOffsetX;
    float subTitleOffsetY = (-(subTitleBounds.height()*0.5+titleBounds.height()+titlePadding_)+bodyRect_.height())*0.5f;

    titleRect_ = bodyRect_.adjusted(-titleOffsetX, titleOffsetY, -titleOffsetX, titleOffsetY);
    subTitleRect_ = bodyRect_.adjusted(-subTitleOffsetX, subTitleOffsetY, -subTitleOffsetX, subTitleOffsetY);

}

void NodeGraphic::initIcon()
{
    icon_ = new NodeIconGraphic(":/node-icons/grid.svg", this);
    // icon_ = new NodeIconGraphic("/home/parker/MyRepos/masters/static/icons/icon-main-white.svg", this);
    
    icon_->setScale(1.0f/icon_->boundingRect().width()*iconScale_);
    // icon_->setScale(0.01);
    icon_->setPos(titleRect_.left()-iconScale_-iconPadding_, -iconScale_*0.5f);
}

void NodeGraphic::initFlagButtons()
{
    displayFlagButton_ = new DisplayFlagButton(this);
    float padding = 2;
    displayFlagButton_->setPos(QPointF(bodyRect_.right()-displayFlagButton_->getWidth()/2.0f-padding, bodyRect_.center().y()));
}

#include <icecream.hpp>
void NodeGraphic::initSockets()
{
    enzo::nt::GeometryOperator& op = enzo::nt::nm().getGeoOperator(opId_);
    IC();
    for(int i=0, max=op.getMaxInputs(); i<max; ++i)
    {
        IC();
        std::cout << "CREATING INPUT SOCKET!\n";
        auto* socketInput = new SocketGraphic(enzo::nt::SocketIOType::Input, opId_, i, this);
        socketInput->setPos(getSocketPosition(i, enzo::nt::SocketIOType::Input));
        inputs_.push_back(socketInput);
    }

    for(int i=0, max=op.getMaxOutputs(); i<max; ++i)
    {
        IC();
        std::cout << "CREATING OUTPUT SOCKET!\n";
        auto* socketOutput = new SocketGraphic(enzo::nt::SocketIOType::Output, opId_, i, this);
        socketOutput->setPos(getSocketPosition(i, enzo::nt::SocketIOType::Output));
        outputs_.push_back(socketOutput);
    }

}

// void setInputEdge(NodeEdgeGraphic* edge, int indx)
// {

// }

// void setOutputEdge(NodeEdgeGraphic* edge, int indx)
// {

// }


// void NodeGraphic::addEdge(NodeEdgeGraphic* edge)
// {
//     edges_.push_back(edge);
// }

SocketGraphic* NodeGraphic::getInput(int indx) const
{
    if(inputs_.size()==0)
    {
        throw std::out_of_range("Can't access inputs when node has no inputs: " + std::to_string(indx)); 
    }
    else if(indx>inputs_.size()-1)
    {
        throw std::out_of_range("Can't access input: " + std::to_string(indx)); 
    }
    return inputs_.at(indx);
}
SocketGraphic* NodeGraphic::getOutput(int indx) const
{
    if(outputs_.size()==0)
    {
        throw std::out_of_range("Can't access outputs when node has no outputs: " + std::to_string(indx)); 
    }
    else if(indx>inputs_.size()-1)
    {
        throw std::out_of_range("Can't access input: " + std::to_string(indx)); 
    }
    return outputs_.at(indx);
}

QRectF NodeGraphic::boundingRect() const
{
    QRectF boundRect = bodyRect_;
    float padding = 0;
    boundRect.adjust(-padding, -padding, padding, padding);
    return boundRect;
}

void NodeGraphic::setSelected(bool selected)
{
    selected_=selected;
    update();
}

bool NodeGraphic::toggleSelected()
{
    bool selected = !selected_;

    setSelected(selected);
    return selected;
}


void NodeGraphic::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // default outline
    QPen defaultPen = Qt::NoPen;
    defaultPen.setWidth(1);
    // outline when selected
    QPen selectedPen = QPen("#edd044");
    // choose outline
    if(isSelected()) painter->setPen(selectedPen);
    else painter->setPen(defaultPen);

    // set fill
    QBrush whiteBrush = QBrush("white");
    painter->setBrush(QBrush(QColor("#1b1b1b")));

    if(selected_) painter->setPen(QPen(QColor("#fee046"), 2));
    painter->drawRoundedRect(bodyRect_, 5, 5);

    painter->setPen(QPen(QColor("white")));

    painter->setFont(titleFont_);
    // painter->setPen(QPen(QColor("#d2d2d2")));
    painter->setPen(QPen(QColor("white")));

    painter->drawText(titleRect_, Qt::AlignLeft, titleText_.c_str());

    painter->setFont(subTitleFont_);
    painter->drawText(subTitleRect_, Qt::AlignLeft, subTitleText_.c_str());

 
}

// void NodeGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
// {
//     QGraphicsItem::mouseMoveEvent(event);
//     updatePositions(event->scenePos());
// }

// void NodeGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
// {
//     updatePositions(event->scenePos());
//     QGraphicsItem::mouseReleaseEvent(event);
// }

void NodeGraphic::updatePositions()
{
    for(int socketIndex=0; socketIndex<inputs_.size(); ++socketIndex)
    {
        inputs_[socketIndex]->posChanged(getSocketScenePosition(socketIndex, enzo::nt::SocketIOType::Input));
    }
    for(int socketIndex=0; socketIndex<outputs_.size(); ++socketIndex)
    {
        outputs_[socketIndex]->posChanged(getSocketScenePosition(socketIndex, enzo::nt::SocketIOType::Output));
    }
}

QPointF NodeGraphic::getSocketPosition(int socketIndex, enzo::nt::SocketIOType socketType)
{
    enzo::nt::GeometryOperator& op = enzo::nt::nm().getGeoOperator(opId_);
    int maxSocketNumber = socketType==enzo::nt::SocketIOType::Input ? op.getMaxInputs() : op.getMaxOutputs();
    float socketSpread = socketSize_*1.5*maxSocketNumber;

    float xPos, yPos;
    xPos = bodyRect_.center().x();
    yPos = socketType == enzo::nt::SocketIOType::Input ? bodyRect_.top() : bodyRect_.bottom();

    xPos += ((socketIndex/static_cast<float>(std::max(maxSocketNumber-1, 1)))-0.5)*2*socketSpread;

    return QPointF(xPos, yPos);
}
QPointF NodeGraphic::getSocketScenePosition(int socketIndex, enzo::nt::SocketIOType socketType)
{
    return this->pos()+getSocketPosition(socketIndex, socketType);
}

enzo::nt::OpId NodeGraphic::getOpId() const
{
    return opId_;
}

void NodeGraphic::setDisplayFlag(bool state)
{
    displayFlagButton_->setEnabled(state);
}


QRectF NodeGraphic::getBodyRect()
{
    return bodyRect_;
}



QVariant NodeGraphic::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    QVariant returnVal = QGraphicsItem::itemChange(change, value);

    if (change == ItemPositionHasChanged) {
        updatePositions();
    };
    
    return returnVal;
}
