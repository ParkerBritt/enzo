#pragma once
#include <QWidget>
#include <iterator>
#include <memory>
#include <qgraphicsitem.h>
#include <qpoint.h>
#include <typeinfo>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Types.h"
#include "Gui/Network/NetworkGraphicsView.h"
#include "Gui/Network/NodeEdgeGraphic.h"
#include "Gui/Network/NetworkGraphicsScene.h"
#include "Gui/Network/NodeGraphic.h"
#include "Gui/Network/SocketGraphic.h"
#include "Gui/Network/FloatingEdgeGraphic.h"
#include <iostream>
#include <QPointer>
#include <unordered_map>
#include <unordered_set>
#include "Gui/Network/TabMenu.h"

class NetworkPanel
: public QWidget
{
public:
    NetworkPanel(QWidget* parent = nullptr);
    void socketClicked(SocketGraphic* socket, QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);
    QSize sizeHint() const override { return QSize(-1, -1); }

    enum class State
    {
        DEFAULT,
        MOVING_NODE
    };
    void createNode(enzo::op::OpInfo opInfo);
    void onOperatorCreated(enzo::nt::OpId opId);
    void clearNetwork();

private:
    QLayout* mainLayout_;
    NetworkGraphicsScene* scene_;
    NetworkGraphicsView* view_;

    std::unordered_map<enzo::nt::OpId, NodeGraphic*> nodeStore_;

    FloatingEdgeGraphic* floatingEdge_=nullptr;
    SocketGraphic* startSocket_=nullptr;

    std::unordered_set<QGraphicsItem*> prevHoverItems_;
    // nodes currently being moved
    std::vector<QGraphicsItem*> moveNodeBuffer;
    QPointF nodeMoveDelta_;

    State state_=State::DEFAULT;
    QPointF leftMouseStart;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void destroyFloatingEdge();
    void deleteEdge(QGraphicsItem* edge);

    enzo::ui::TabMenu* tabMenu_;

    void leftMousePressed(QMouseEvent* event);

    void moveNodes(QPointF pos);

    template<typename T>
    bool isType(QGraphicsItem* item)
    {
        return item && typeid(*item)==typeid(T);
    }

    template<typename T>
    QGraphicsItem* itemOfType(QList<QGraphicsItem*> items)
    {
        for(QGraphicsItem* item : items)
        {
            if(item && typeid(*item)==typeid(T))
            {
                return item;
            }
        }
        return nullptr;
    }

    template<typename T>
    QGraphicsItem* itemOfType(std::unordered_set<QGraphicsItem*> items)
    {
        for(QGraphicsItem* item : items)
        {
            if(item && typeid(*item)==typeid(T))
            {
                return item;
            }
        }
        return nullptr;
    }

    template<typename T>
    QGraphicsItem* closestItemOfType(QList<QGraphicsItem*> items, QPointF centerPos)
    {
        std::vector <QGraphicsItem*> filteredItems;
        for(QGraphicsItem* item : items)
        {
            if(item && typeid(*item)==typeid(T))
            {
                filteredItems.push_back(item);
            }
        }

        if(filteredItems.size()==0) return nullptr;
        if(filteredItems.size()==1) return filteredItems.at(0);

        QGraphicsItem* closestItem=filteredItems.at(0);
        float closestDist;
        {
            QPointF itemPos;
            if constexpr (std::is_base_of<NodeEdgeGraphic, T>::value)
            {
                itemPos = static_cast<NodeEdgeGraphic*>(closestItem)->closestPoint(centerPos);
            }
            else
            {
                itemPos = closestItem->scenePos();
            }
            closestDist = QLineF(itemPos, centerPos).length();
        }

        for(size_t i=1; i<filteredItems.size(); ++i)
        {
            QGraphicsItem* item = filteredItems.at(i);

            QPointF itemPos;
            if constexpr (std::is_base_of<NodeEdgeGraphic, T>::value)
            {
                itemPos = static_cast<NodeEdgeGraphic*>(item)->closestPoint(centerPos);
            }
            else
            {
                itemPos = item->scenePos();
            }
            auto currentDist = QLineF(itemPos, centerPos).length();
            if(currentDist < closestDist)
            {
                closestItem = item;
                closestDist = currentDist;
            }

        }

        return closestItem;
    }


protected:
    void resizeEvent(QResizeEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool focusNextPrevChild(bool) override;
};
