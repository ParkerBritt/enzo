#pragma once

#include "Gui/Network/NodeListModel.h"
#include <QAbstractListModel>
#include <QColor>
#include <QPointF>
#include <QQuickItem>
#include <vector>

namespace enzo::ui {

/// @brief Draws every node link of the network into one scene graph geometry node.
///
/// Each link is a cubic bezier from a source node's bottom center output to a
/// target node's top center input. The layer owns no graph state, it reads the
/// node and link models the view-model drives and repaints when they change.
class NodeLinkLayer : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(NodeListModel* nodes READ nodes WRITE setNodes NOTIFY nodesChanged)
    Q_PROPERTY(QAbstractListModel* links READ links WRITE setLinks NOTIFY linksChanged)
    Q_PROPERTY(QColor linkColor MEMBER linkColor_ NOTIFY linkColorChanged)

    // The in-progress link dragged from a fixed port to the cursor.
    Q_PROPERTY(
        bool floatingActive READ floatingActive WRITE setFloatingActive NOTIFY floatingChanged
    )
    Q_PROPERTY(
        QPointF floatingOutput READ floatingOutput WRITE setFloatingOutput NOTIFY floatingChanged
    )
    Q_PROPERTY(
        QPointF floatingInput READ floatingInput WRITE setFloatingInput NOTIFY floatingChanged
    )

  public:
    /// One node link as the two port points its curve spans.
    struct Link
    {
        QPointF output;
        QPointF input;
    };

    explicit NodeLinkLayer(QQuickItem* parent = nullptr);

    NodeListModel* nodes() const;
    void setNodes(NodeListModel* model);

    QAbstractListModel* links() const;
    void setLinks(QAbstractListModel* model);

    bool floatingActive() const;
    void setFloatingActive(bool active);

    QPointF floatingOutput() const;
    void setFloatingOutput(QPointF point);

    QPointF floatingInput() const;
    void setFloatingInput(QPointF point);

  Q_SIGNALS:
    void nodesChanged();
    void linksChanged();
    void linkColorChanged();
    void floatingChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

  private:
    /// @brief Connects a model's change signals to a repaint.
    void connectForRepaint(QAbstractItemModel* model);

    /// @brief Resolves each link row into the two port points its curve spans.
    std::vector<Link> collectLinks() const;

    NodeListModel* nodes_ = nullptr;
    QAbstractListModel* links_ = nullptr;
    QColor linkColor_{"#3a3a46"};
    bool floatingActive_ = false;
    QPointF floatingOutput_;
    QPointF floatingInput_;
};

} // namespace enzo::ui
