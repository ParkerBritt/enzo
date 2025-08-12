#include "Gui/Network/Network.h"
#include "Engine/Operator/GeometryConnection.h"
#include "Engine/Operator/GeometryOperator.h"
#include "Engine/Operator/OperatorTable.h"
#include "Engine/Types.h"
#include "Gui/Network/DisplayFlagButton.h"
#include "Gui/Network/NodeEdgeGraphic.h"
#include "Gui/Network/NetworkGraphicsView.h"
#include "Gui/Network/NetworkGraphicsScene.h"
#include "Gui/Network/NodeGraphic.h"
#include "Gui/Network/FloatingEdgeGraphic.h"
#include "Gui/Network/SocketGraphic.h"
#include "Engine/Network/NetworkManager.h"
#include <memory>
#include <qboxlayout.h>
#include <QPushButton>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <qgraphicsitem.h>
#include <qnamespace.h>
#include <QLine>
#include <stdexcept>
#include "Gui/Network/TabMenu.h"
#include <icecream.hpp>

using namespace enzo;

Network::Network(QWidget* parent)
: QWidget(parent)
{

    mainLayout_ = new QVBoxLayout(this);
    // mainLayout_->setContentsMargins(0,0,0,0);
    // this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    tabMenu_ = new enzo::ui::TabMenu(this);

    this->setLayout(mainLayout_); 


    scene_ = new NetworkGraphicsScene();
    view_ = new NetworkGraphicsView(this, this, scene_);


    mainLayout_->addWidget(view_);

}

void Network::resizeEvent(QResizeEvent *event)
{
    QPainterPath path;
    constexpr float radius = 10;
    path.addRoundedRect(mainLayout_->contentsRect(), radius, radius);
    QRegion region = QRegion(path.toFillPolygon().toPolygon());
    this->setMask(region);
}

void Network::deleteEdge(QGraphicsItem* edge)
{
    std::cout << "----\ndeleting edge\n";
    if(!edge) return;
    if(auto it = prevHoverItems_.find(edge); it != prevHoverItems_.end())
    {
        prevHoverItems_.erase(it);
    }
    // NOTE: deleting edge kept giving me segmentation faults
    // I coundn't figure it out so I'm just leaving it for now
    // delete edge;
    static_cast<NodeEdgeGraphic*>(edge)->cleanUp();
    std::cout << "finished deleting edge\n----\n";
}

void Network::mousePressEvent(QMouseEvent *event)
{
    if( event->buttons() & Qt::LeftButton)
    {
        leftMousePressed(event);
    }

}

void Network::leftMousePressed(QMouseEvent *event)
{
    std::cout << "LEFT MOUSE PRESSED\n";
    Qt::KeyboardModifiers mods = event->modifiers();
    leftMouseStart = event->pos();

    QList<QGraphicsItem*> clickedItems = view_->items(event->pos());
    QGraphicsItem* clickedSocket = itemOfType<SocketGraphic>(clickedItems);


    // delete edges
    if(
        QGraphicsItem* clickedEdge = closestItemOfType<NodeEdgeGraphic>(clickedItems, view_->mapToScene(event->pos()));
        mods & Qt::ControlModifier && clickedEdge
    )
    {
        deleteEdge(clickedEdge);
    }
    // socket logic
    else if(clickedSocket)
    {
        // find closest socket
        clickedSocket = closestItemOfType<SocketGraphic>(clickedItems, view_->mapToScene(event->pos()));
        if(clickedSocket)
        {
            socketClicked(static_cast<SocketGraphic*>(clickedSocket), event);
        }
    }
    // floating edge
    else if(floatingEdge_)
    {
        destroyFloatingEdge();
    }
    else if(QGraphicsItem* clickedNode = itemOfType<NodeGraphic>(clickedItems))
    {
        nodeMoveDelta_=clickedNode->pos()-view_->mapToScene(event->pos());
        std::cout << "move delta: " << nodeMoveDelta_.x() << " " << nodeMoveDelta_.y() << "\n";
        state_=State::MOVING_NODE;
        moveNodeBuffer.clear();
        moveNodeBuffer.push_back(clickedNode);
    }

}




void Network::socketClicked(SocketGraphic* socket, QMouseEvent *event)
{
    std::cout << "socket clicked\n";
    // clicked first socket
    if(!floatingEdge_)
    {
        startSocket_=socket;
        std::cout << "creating floating edge\n";
        floatingEdge_ = new FloatingEdgeGraphic(socket);
        scene_->addItem(floatingEdge_);
        floatingEdge_->setFloatPos(view_->mapToScene(event->pos()));
    }
    // clicked second socket
    // connect to opposite type
    else if (
        socket->getIO()!=startSocket_->getIO() &&
        startSocket_->getOpId()!=socket->getOpId()
    )
    {

        // order sockets in relation to data flow
        // the input node is the node the data flows from
        auto inputNodeSocket = socket->getIO()==enzo::nt::SocketIOType::Output ? socket : startSocket_;
        // the output node is the node the data flows to
        auto outputNodeSocket = startSocket_->getIO()==enzo::nt::SocketIOType::Input ? startSocket_ : socket;

        nt::GeometryOperator& geoOp = enzo::nt::nm().getGeoOperator(outputNodeSocket->getOpId());

        std::cout << "CONNECTING opid: " << inputNodeSocket->getOpId() << " -> " << outputNodeSocket->getOpId() << "\n";


        nt::connectOperators(inputNodeSocket->getOpId(), inputNodeSocket->getIndex(), outputNodeSocket->getOpId(), outputNodeSocket->getIndex());


        NodeEdgeGraphic* newEdge = new NodeEdgeGraphic(outputNodeSocket, inputNodeSocket);

        newEdge->setPos(outputNodeSocket->scenePos(), inputNodeSocket->scenePos());
        scene_->addItem(newEdge);
        destroyFloatingEdge();
    }
}

void Network::destroyFloatingEdge()
{
    if(floatingEdge_)
    {
        scene_->removeItem(floatingEdge_);
        delete floatingEdge_;
        floatingEdge_=nullptr;
    }
}



void Network::mouseMoved(QMouseEvent *event)
{
    // cache and reset prev hover items
    std::unordered_set<QGraphicsItem*> prevHoverItems = prevHoverItems_;
    prevHoverItems_.clear();
    // handle previous items
    for(QGraphicsItem* item : prevHoverItems)
    {
        if(isType<SocketGraphic>(item))
        {
            static_cast<SocketGraphic*>(item)->setHover(false);
        }
        if(isType<NodeEdgeGraphic>(item))
        {
            static_cast<NodeEdgeGraphic*>(item)->setDeleteHighlight(false);
        }
    }

    // modifiers
    Qt::KeyboardModifiers mods = event->modifiers();
    bool ctrlMod = mods & Qt::ControlModifier;

    QList<QGraphicsItem*> hoverItems = view_->items(event->pos());


    if(state_==State::MOVING_NODE)
    {
        moveNodes(view_->mapToScene(event->pos())+nodeMoveDelta_);
        return;
    }

    if(floatingEdge_)
    {
        if(
            SocketGraphic* hoverSocket = static_cast<SocketGraphic*>(closestItemOfType<SocketGraphic>(hoverItems, view_->mapToScene(event->pos())));
            hoverSocket &&
            hoverSocket!=startSocket_ &&
            hoverSocket->getIO()!=startSocket_->getIO() &&
            hoverSocket->getOpId()!=startSocket_->getOpId()
            
        )
        {
            floatingEdge_->setFloatPos(hoverSocket->scenePos());
        }
        else
        {
            floatingEdge_->setFloatPos(view_->mapToScene(event->pos()));
        }
        event->accept();
        return;
    }

    QGraphicsItem* hoverEdge = closestItemOfType<NodeEdgeGraphic>(hoverItems, view_->mapToScene(event->pos()));

    // set node edge color
    if(ctrlMod && hoverEdge)
    {
        if(event->buttons() & Qt::LeftButton)
        {
            deleteEdge(hoverEdge);
        }
        else
        {
            static_cast<NodeEdgeGraphic*>(hoverEdge)->setDeleteHighlight(true);
            prevHoverItems_.insert(hoverEdge);
        }
    }

    // highlight hovered socket
    else if(auto hoverSocket = closestItemOfType<SocketGraphic>(hoverItems, view_->mapToScene(event->pos())))
    {
        static_cast<SocketGraphic*>(hoverSocket)->setHover(true);
        prevHoverItems_.insert(hoverSocket);
    }
}

void Network::moveNodes(QPointF pos)
{

    for(auto node : moveNodeBuffer)
    {
        node->setPos(pos);
    }
}


void Network::keyPressEvent(QKeyEvent *event)
{
    // modifiers
    Qt::KeyboardModifiers mods = event->modifiers();
    bool ctrlMod = mods & Qt::ControlModifier;

    // get pos
    QPoint globalPos = QCursor::pos();
    QPoint widgetPos = mapFromGlobal(globalPos);
    QPointF viewPos = view_->mapToScene(widgetPos);
    

    QList<QGraphicsItem*> hoverItems = view_->items(widgetPos);

    // edge detection
    switch(event->key())
    {

        case( Qt::Key_Control):
        {
            QGraphicsItem* hoverItem = itemOfType<NodeEdgeGraphic>(hoverItems);
            if(hoverItem!=nullptr)
            {
                static_cast<NodeEdgeGraphic*>(hoverItem)->setDeleteHighlight(true);

                // deselect sockets
                for(auto item : hoverItems)
                {
                    if(isType<SocketGraphic>(item))
                    {
                        static_cast<SocketGraphic*>(item)->setHover(false);
                    }
                }
                prevHoverItems_.insert(hoverItem);
            }
            break;
        }
        case(Qt::Key_Escape):
        {
            destroyFloatingEdge();
            break;
        }
        case(Qt::Key_Tab):
        {
            tabMenu_->showOnMouse();
            break;
        }
        // case(Qt::Key_G):
        // {
        //     auto opInfo = op::OperatorTable::getOpInfo("transform");
        //     if(!opInfo.has_value()) {throw std::runtime_error("Couldn't find op info for: " + )}
        //     if(
        //         opInfo.has_value() &&
        //         auto newNode = createNode(opInfo)
        //         )
        //     {
        //         newNode->setPos(viewPos);
        //     }

        //     break;
        // }
        // case(Qt::Key_F):
        // {
        //     if(auto newNode = createNode(op::OperatorTable::getOpInfo("house")))
        //     {
        //         newNode->setPos(viewPos);
        //     }

        //     break;
        // }
    }
}

NodeGraphic* Network::createNode(op::OpInfo opInfo)
{
    if(nt::OpId id = enzo::nt::nm().addOperator(opInfo))
    {
        NodeGraphic* newNode = new NodeGraphic(id);
        QPointF cursorPos = view_->mapToScene(mapFromGlobal(QCursor::pos()));
        newNode->setPos(cursorPos);

        scene_->addItem(newNode);
        nodeStore_.emplace(id, newNode);
        
        return newNode;
    }
    else
    {
        return nullptr;
    }
}


void Network::keyReleaseEvent(QKeyEvent *event)
{
    // modifiers
    Qt::KeyboardModifiers mods = event->modifiers();
    bool ctrlMod = mods & Qt::ControlModifier;

    // handle previous items
    for(QGraphicsItem* item : prevHoverItems_)
    {
        if(
            event->key() == Qt::Key_Control &&
            isType<NodeEdgeGraphic>(item)
        )
        {
            static_cast<NodeEdgeGraphic*>(item)->setDeleteHighlight(false);
        }

    }
}

void Network::mouseReleaseEvent(QMouseEvent *event)
{
    // std::cout << "----\nMOUSE RELEASED\n---\n";
    QList<QGraphicsItem*> hoverItems = view_->items(event->pos());
    QGraphicsItem* hoverSocket = itemOfType<SocketGraphic>(hoverItems);
    if(event->button() == Qt::LeftButton)
    {
        // display flag
        if(
            QGraphicsItem* clickedDisplayFlag = itemOfType<DisplayFlagButton>(hoverItems);
            clickedDisplayFlag &&
            QLineF(event->pos(), leftMouseStart).length()<5.0f
        )
        {
            enzo::nt::NetworkManager& nm = enzo::nt::nm();
            NodeGraphic* clickedNode = static_cast<NodeGraphic*>(itemOfType<NodeGraphic>(hoverItems));
            enzo::nt::OpId opId = clickedNode->getOpId();
            if(auto prevDisplayOpId = nm.getDisplayOp(); prevDisplayOpId)
            {
                NodeGraphic* prevDisplayNode = nodeStore_.at(*prevDisplayOpId);
                prevDisplayNode->setDisplayFlag(false);
            }
            static_cast<DisplayFlagButton*>(clickedDisplayFlag)->setEnabled(true);
            nm.setDisplayOp(opId);
        }
        if(state_==State::MOVING_NODE)
        {
            moveNodeBuffer.clear();
            state_=State::DEFAULT;

            // select node
            if(
                QGraphicsItem* clickedNode = itemOfType<NodeGraphic>(hoverItems);
                clickedNode &&
                QLineF(event->pos(), leftMouseStart).length()<5.0f)
            {
                NodeGraphic* node = static_cast<NodeGraphic*>(clickedNode);
                // deselect previous
                auto selectedNodeIds = enzo::nt::nm().getSelectedNodes();
                for(auto nodeId : selectedNodeIds)
                {
                    NodeGraphic* nodeGraphic = nodeStore_.at(nodeId);
                    nodeGraphic->setSelected(false);
                }

                bool selected = node->toggleSelected();
                enzo::nt::nm().setSelectedNode(node->getOpId(), selected);
            }

        }
        else if(floatingEdge_ && hoverSocket)
        {
            hoverSocket = closestItemOfType<SocketGraphic>(hoverItems, view_->mapToScene(event->pos()));
            if(hoverSocket)
            {
                socketClicked(static_cast<SocketGraphic*>(hoverSocket), event);
            }
        }
    }
}

bool Network::focusNextPrevChild(bool) { return false; }
