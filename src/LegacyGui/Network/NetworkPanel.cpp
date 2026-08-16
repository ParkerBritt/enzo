#include "LegacyGui/Network/NetworkPanel.h"
#include "Engine/Core/Types.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/UndoRedo/ChangeDisplayFlagCommand.h"
#include "Engine/UndoRedo/ChangeSelectionCommand.h"
#include "LegacyGui/Network/DisplayFlagButton.h"
#include "LegacyGui/Network/FloatingEdgeGraphic.h"
#include "LegacyGui/Network/NetworkGraphicsScene.h"
#include "LegacyGui/Network/NetworkGraphicsView.h"
#include "LegacyGui/Network/NodeEdgeGraphic.h"
#include "LegacyGui/Network/NodeGraphic.h"
#include "LegacyGui/Network/SocketGraphic.h"
#include "LegacyGui/Network/TabMenu.h"
#include <QApplication>
#include <QGraphicsItem>
#include <QLine>
#include <QMouseEvent>
#include <QPushButton>
#include <icecream.hpp>
#include <memory>
#include <qboxlayout.h>
#include <qgraphicsitem.h>
#include <qnamespace.h>
#include <stdexcept>

using namespace enzo;

NetworkPanel::NetworkPanel(QWidget* parent) : Panel(parent)
{

    mainLayout_ = new QVBoxLayout(this);
    // mainLayout_->setContentsMargins(0,0,0,0);
    // this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    tabMenu_ = new enzo::ui::TabMenu(this);

    this->setLayout(mainLayout_);

    scene_ = new NetworkGraphicsScene();
    view_ = new NetworkGraphicsView(this, this, scene_);

    mainLayout_->addWidget(view_);

    // Node position changed
    enzo::nt::nm().nodePositionChanged.connect([this](enzo::nt::NodeId nodeId, enzo::Vector2 pos) {
        if (auto it = nodeStore_.find(nodeId); it != nodeStore_.end())
        {
            it->second->setPos(pos.x(), pos.y());
        }
    });

    // Nodes removed
    enzo::nt::nm().nodeRemoved.connect([this](enzo::nt::NodeId nodeId) {
        if (auto it = nodeStore_.find(nodeId); it != nodeStore_.end())
        {
            NodeGraphic* node = it->second;
            nodeStore_.erase(it);
            // Hold the node alive until the collapse finishes, then drop it
            node->animateRemoval([this, node] {
                scene_->removeItem(node);
                delete node;
            });
        }
    });

    // Display nodes changed
    enzo::nt::nm().displayNodeChanged.connect([this](std::optional<enzo::nt::NodeId> nodeId) {
        for (auto& [id, node] : nodeStore_)
        {
            node->setDisplayFlag(nodeId.has_value() && id == *nodeId);
        }
    });

    // Selected nodes changed
    enzo::nt::nm().selectedNodesChanged.connect([this](std::vector<enzo::nt::NodeId> selectedIds) {
        // TODO: potentially slow iterating through every node
        for (auto& [id, node] : nodeStore_)
        {
            node->setSelected(false);
        }
        for (enzo::nt::NodeId id : selectedIds)
        {
            if (auto it = nodeStore_.find(id); it != nodeStore_.end())
            {
                it->second->setSelected(true);
            }
        }
    });
}

void NetworkPanel::deleteEdge(QGraphicsItem* edge)
{
    if (!edge) return;

    // Disconnect in the engine, the graphic is torn down by onConnectionRemoved
    enzo::nt::nm().disconnectNodes(static_cast<NodeEdgeGraphic*>(edge)->getConnection());
}

void NetworkPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        leftMousePressed(event);
    }
}

void NetworkPanel::leftMousePressed(QMouseEvent* event)
{
    std::cout << "LEFT MOUSE PRESSED\n";
    Qt::KeyboardModifiers mods = event->modifiers();
    leftMouseStart = event->pos();

    QList<QGraphicsItem*> clickedItems = view_->items(event->pos());
    QGraphicsItem* clickedSocket = itemOfType<SocketGraphic>(clickedItems);

    // delete edges
    if (QGraphicsItem* clickedEdge =
            closestItemOfType<NodeEdgeGraphic>(clickedItems, view_->mapToScene(event->pos()));
        mods & Qt::ControlModifier && clickedEdge)
    {
        deleteEdge(clickedEdge);
    }
    // socket logic
    else if (clickedSocket)
    {
        // find closest socket
        clickedSocket =
            closestItemOfType<SocketGraphic>(clickedItems, view_->mapToScene(event->pos()));
        if (clickedSocket)
        {
            socketClicked(static_cast<SocketGraphic*>(clickedSocket), event);
        }
    }
    // floating edge
    else if (floatingEdge_)
    {
        destroyFloatingEdge();
    }
    else if (QGraphicsItem* clickedNode = itemOfType<NodeGraphic>(clickedItems))
    {
        state_ = State::MOUSE_DOWN_NODE;
        moveNodeBuffer.clear();
        moveNodeBuffer.push_back(clickedNode);
    }
}

void NetworkPanel::socketClicked(SocketGraphic* socket, QMouseEvent* event)
{
    std::cout << "socket clicked\n";
    // clicked first socket
    if (!floatingEdge_)
    {
        startSocket_ = socket;
        std::cout << "creating floating edge\n";
        floatingEdge_ = new FloatingEdgeGraphic(socket);
        scene_->addItem(floatingEdge_);
        floatingEdge_->setFloatPos(view_->mapToScene(event->pos()));
    }
    // clicked second socket
    // connect to opposite type
    else if (socket->getIO() != startSocket_->getIO() &&
             startSocket_->getNodeId() != socket->getNodeId())
    {

        // order sockets in relation to data flow
        // the input node is the node the data flows from
        auto inputNodeSocket =
            socket->getIO() == enzo::nt::SocketIOType::Output ? socket : startSocket_;
        // the output node is the node the data flows to
        auto outputNodeSocket =
            startSocket_->getIO() == enzo::nt::SocketIOType::Input ? startSocket_ : socket;

        nt::Node& node = enzo::nt::nm().getNode(outputNodeSocket->getNodeId());

        std::cout << "CONNECTING opid: " << inputNodeSocket->getNodeId() << " -> "
                  << outputNodeSocket->getNodeId() << "\n";

        nt::nm().connectNodes(
            inputNodeSocket->getNodeId(),
            inputNodeSocket->getIndex(),
            outputNodeSocket->getNodeId(),
            outputNodeSocket->getIndex()
        );

        destroyFloatingEdge();
    }
}

void NetworkPanel::destroyFloatingEdge()
{
    if (floatingEdge_)
    {
        scene_->removeItem(floatingEdge_);
        delete floatingEdge_;
        floatingEdge_ = nullptr;
    }
}

void NetworkPanel::mouseMoved(QMouseEvent* event)
{
    // cache and reset prev hover items
    std::unordered_set<QGraphicsItem*> prevHoverItems = prevHoverItems_;
    prevHoverItems_.clear();
    // handle previous items
    for (QGraphicsItem* item : prevHoverItems)
    {
        if (isType<SocketGraphic>(item))
        {
            static_cast<SocketGraphic*>(item)->setHover(false);
        }
        if (isType<NodeEdgeGraphic>(item))
        {
            static_cast<NodeEdgeGraphic*>(item)->setDeleteHighlight(false);
        }
    }

    // modifiers
    Qt::KeyboardModifiers mods = event->modifiers();
    bool ctrlMod = mods & Qt::ControlModifier;

    QList<QGraphicsItem*> hoverItems = view_->items(event->pos());

    if (state_ == State::MOUSE_DOWN_NODE)
    {
        if (QLineF(event->pos(), leftMouseStart).length() > 4.0f)
        {
            state_ = State::MOVING_NODE;
            nodeMoveDelta_ = moveNodeBuffer.front()->pos() - view_->mapToScene(event->pos());
        }
        return;
    }

    if (state_ == State::MOVING_NODE)
    {
        moveNodes(view_->mapToScene(event->pos()) + nodeMoveDelta_);
        return;
    }

    if (floatingEdge_)
    {
        if (
            SocketGraphic* hoverSocket = static_cast<SocketGraphic*>(
                closestItemOfType<SocketGraphic>(hoverItems, view_->mapToScene(event->pos()))
            );
            hoverSocket && hoverSocket != startSocket_ &&
            hoverSocket->getIO() != startSocket_->getIO() &&
            hoverSocket->getNodeId() != startSocket_->getNodeId()

        )
        {
            floatingEdge_->setFloatPos(hoverSocket->scenePos());
            hoverSocket->setHover(true);
            prevHoverItems_.insert(hoverSocket);
        }
        else
        {
            floatingEdge_->setFloatPos(view_->mapToScene(event->pos()));
        }
        event->accept();
        return;
    }

    QGraphicsItem* hoverEdge =
        closestItemOfType<NodeEdgeGraphic>(hoverItems, view_->mapToScene(event->pos()));

    // set node edge color
    if (ctrlMod && hoverEdge)
    {
        if (event->buttons() & Qt::LeftButton)
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
    else if (auto hoverSocket =
                 closestItemOfType<SocketGraphic>(hoverItems, view_->mapToScene(event->pos())))
    {
        static_cast<SocketGraphic*>(hoverSocket)->setHover(true);
        prevHoverItems_.insert(hoverSocket);
    }
}

void NetworkPanel::moveNodes(QPointF pos)
{

    for (auto node : moveNodeBuffer)
    {
        node->setPos(pos);
    }
}

void NetworkPanel::keyPressEvent(QKeyEvent* event)
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
    switch (event->key())
    {

    case (Qt::Key_Control):
    {
        QGraphicsItem* hoverItem = itemOfType<NodeEdgeGraphic>(hoverItems);
        if (hoverItem != nullptr)
        {
            static_cast<NodeEdgeGraphic*>(hoverItem)->setDeleteHighlight(true);

            // deselect sockets
            for (auto item : hoverItems)
            {
                if (isType<SocketGraphic>(item))
                {
                    static_cast<SocketGraphic*>(item)->setHover(false);
                }
            }
            prevHoverItems_.insert(hoverItem);
        }
        break;
    }
    case (Qt::Key_Escape):
    {
        destroyFloatingEdge();
        break;
    }
    case (Qt::Key_Tab):
    {
        tabMenu_->showOnMouse();
        break;
    }
    case (Qt::Key_Z):
    {
        if (ctrlMod) enzo::nt::nm().undoStack().undo();
        break;
    }
    case (Qt::Key_Y):
    {
        if (ctrlMod) enzo::nt::nm().undoStack().redo();
        break;
    }
    case (Qt::Key_Delete):
    case (Qt::Key_Backspace):
    {
        auto selectedIds = enzo::nt::nm().getSelectedNodes();
        for (auto nodeId : selectedIds)
        {
            enzo::nt::nm().deleteNode(nodeId);
        }
        break;
    }
        // case(Qt::Key_G):
        // {
        //     auto nodeType = nt::NodeTypeTable::getNodeType("transform");
        //     if(!nodeType.has_value()) {throw std::runtime_error("Couldn't find node info for: " + )}
        //     if(
        //         nodeType.has_value() &&
        //         auto newNode = createNode(nodeType)
        //         )
        //     {
        //         newNode->setPos(viewPos);
        //     }

        //     break;
        // }
        // case(Qt::Key_F):
        // {
        //     if(auto newNode = createNode(nt::NodeTypeTable::getNodeType("house")))
        //     {
        //         newNode->setPos(viewPos);
        //     }

        //     break;
        // }
    }
}

void NetworkPanel::createNode(nt::NodeType nodeType)
{
    QPointF cursorPos = view_->mapToScene(mapFromGlobal(QCursor::pos()));
    enzo::nt::nm().createNode(
        nodeType,
        "",
        {static_cast<float>(cursorPos.x()), static_cast<float>(cursorPos.y())}
    );
}

void NetworkPanel::clearNetwork()
{
    destroyFloatingEdge();
    scene_->clear();
    nodeStore_.clear();
    moveNodeBuffer.clear();
    prevHoverItems_.clear();
    state_ = State::DEFAULT;
}

void NetworkPanel::onNodeCreated(enzo::nt::NodeId nodeId)
{
    auto& node = enzo::nt::nm().getNode(nodeId);
    auto pos = node.getPosition();

    NodeGraphic* newNode = new NodeGraphic(nodeId);
    newNode->setPos(pos.x(), pos.y());

    scene_->addItem(newNode);
    nodeStore_.emplace(nodeId, newNode);

    newNode->animatePlacement();
}

void NetworkPanel::onConnectionCreated(enzo::nt::Connection connection)
{
    NodeGraphic* sourceNode = nodeStore_.at(connection.sourceNode);
    NodeGraphic* targetNode = nodeStore_.at(connection.targetNode);

    SocketGraphic* sourceSocket = sourceNode->getOutput(connection.sourceOutput);
    SocketGraphic* targetSocket = targetNode->getInput(connection.targetInput);

    // Derive endpoints from node geometry so the placement scale animation does not offset the edge
    QPointF targetSocketPos =
        targetNode->getSocketScenePosition(connection.targetInput, enzo::nt::SocketIOType::Input);
    QPointF sourceSocketPos =
        sourceNode->getSocketScenePosition(connection.sourceOutput, enzo::nt::SocketIOType::Output);

    NodeEdgeGraphic* edge = new NodeEdgeGraphic(targetSocket, sourceSocket, connection);
    edge->setPos(targetSocketPos, sourceSocketPos);
    scene_->addItem(edge);

    edgeStore_[connection] = edge;
}

void NetworkPanel::onConnectionRemoved(enzo::nt::Connection connection)
{
    auto it = edgeStore_.find(connection);
    if (it == edgeStore_.end()) return;

    NodeEdgeGraphic* edge = it->second;
    edgeStore_.erase(it);

    prevHoverItems_.erase(edge);
    edge->remove();
    delete edge;
}

void NetworkPanel::keyReleaseEvent(QKeyEvent* event)
{
    // modifiers
    Qt::KeyboardModifiers mods = event->modifiers();
    bool ctrlMod = mods & Qt::ControlModifier;

    // handle previous items
    for (QGraphicsItem* item : prevHoverItems_)
    {
        if (event->key() == Qt::Key_Control && isType<NodeEdgeGraphic>(item))
        {
            static_cast<NodeEdgeGraphic*>(item)->setDeleteHighlight(false);
        }
    }
}

void NetworkPanel::mouseReleaseEvent(QMouseEvent* event)
{
    // std::cout << "----\nMOUSE RELEASED\n---\n";
    QList<QGraphicsItem*> hoverItems = view_->items(event->pos());
    QGraphicsItem* hoverSocket = itemOfType<SocketGraphic>(hoverItems);
    if (event->button() == Qt::LeftButton)
    {
        // display flag
        if (itemOfType<DisplayFlagButton>(hoverItems) &&
            QLineF(event->pos(), leftMouseStart).length() < 5.0f)
        {
            NodeGraphic* clickedNode =
                static_cast<NodeGraphic*>(itemOfType<NodeGraphic>(hoverItems));
            enzo::nt::NodeId nodeId = clickedNode->getNodeId();
            auto cmd = std::make_unique<enzo::nt::ChangeDisplayFlagCommand>(
                enzo::nt::nm().getDisplayNode(),
                nodeId
            );
            enzo::nt::nm().undoStack().push(std::move(cmd));
            enzo::nt::nm().setDisplayNode(nodeId);
        }
        if (state_ == State::MOUSE_DOWN_NODE)
        {
            // Move threshold was never exceeded so registered as click
            // Handle node selection
            if (QGraphicsItem* clickedNode = itemOfType<NodeGraphic>(hoverItems))
            {
                // Get selected nodes
                NodeGraphic* node = static_cast<NodeGraphic*>(clickedNode);
                enzo::nt::NodeId nodeId = node->getNodeId();
                std::vector<enzo::nt::NodeId> prev(enzo::nt::nm().getSelectedNodes());

                // Get modifiers
                bool isCurrentlySelected = std::find(prev.begin(), prev.end(), nodeId) != prev.end();
                bool ctrlHeld = QApplication::keyboardModifiers() & Qt::ControlModifier;

                if (ctrlHeld || !isCurrentlySelected)
                {
                    // Toggle selection if ctrl held
                    // Otherwise only allow selection, not deselection
                    std::vector<enzo::nt::NodeId> next = ctrlHeld && isCurrentlySelected
                                                           ? std::vector<enzo::nt::NodeId>{}
                                                           : std::vector<enzo::nt::NodeId>{nodeId};

                    // Setup undo
                    auto cmd = std::make_unique<enzo::nt::ChangeSelectionCommand>(prev, next);
                    enzo::nt::nm().undoStack().push(std::move(cmd));

                    // Update selection
                    enzo::nt::nm().setSelectedNodes(next);
                }
            }
            moveNodeBuffer.clear();
            state_ = State::DEFAULT;
        }
        else if (state_ == State::MOVING_NODE)
        {
            for (auto* item : moveNodeBuffer)
            {
                auto* node = static_cast<NodeGraphic*>(item);
                QPointF p = node->pos();
                enzo::nt::nm().moveNode(
                    node->getNodeId(),
                    {static_cast<float>(p.x()), static_cast<float>(p.y())}
                );
            }
            moveNodeBuffer.clear();
            state_ = State::DEFAULT;
        }
        else if (floatingEdge_ && hoverSocket)
        {
            hoverSocket =
                closestItemOfType<SocketGraphic>(hoverItems, view_->mapToScene(event->pos()));
            if (hoverSocket)
            {
                socketClicked(static_cast<SocketGraphic*>(hoverSocket), event);
            }
        }
    }
}

bool NetworkPanel::focusNextPrevChild(bool) { return false; }
