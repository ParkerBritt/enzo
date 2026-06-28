#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QHash>
#include <QPointF>
#include <QQuickItem>
#include <QVariantMap>
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

    Q_PROPERTY(QAbstractListModel* nodes READ nodes WRITE setNodes NOTIFY nodesChanged)
    Q_PROPERTY(QAbstractListModel* links READ links WRITE setLinks NOTIFY linksChanged)
    Q_PROPERTY(QColor linkColor MEMBER linkColor_ NOTIFY linkColorChanged)
    Q_PROPERTY(qreal nodeWidth MEMBER nodeWidth_ NOTIFY nodeMetricsChanged)
    Q_PROPERTY(qreal nodeHeight MEMBER nodeHeight_ NOTIFY nodeMetricsChanged)

    // The in-progress link dragged from a fixed port to the cursor.
    Q_PROPERTY(bool floatingActive READ floatingActive WRITE setFloatingActive NOTIFY floatingChanged)
    Q_PROPERTY(QPointF floatingOutput READ floatingOutput WRITE setFloatingOutput NOTIFY floatingChanged)
    Q_PROPERTY(QPointF floatingInput READ floatingInput WRITE setFloatingInput NOTIFY floatingChanged)

  public:
    /// One node link as the two port points its curve spans.
    struct Link
    {
        QPointF output;
        QPointF input;
    };

    /// A node's graph position paired with how many ports each edge carries.
    struct NodeGeometry
    {
        QPointF position;
        int inputSlotCount = 0;
        int outputSlotCount = 0;
    };

    explicit NodeLinkLayer(QQuickItem* parent = nullptr);

    QAbstractListModel* nodes() const;
    void setNodes(QAbstractListModel* model);

    QAbstractListModel* links() const;
    void setLinks(QAbstractListModel* model);

    /// @brief Returns the port nearest a canvas point, for snapping a dropped link.
    /// @param wantOutput Searches output ports when true, input ports when false.
    /// @return A {opId, slot, x, y} map, empty when no port is within snapping range.
    Q_INVOKABLE QVariantMap portAt(QPointF canvasPoint, bool wantOutput) const;

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
    void nodeMetricsChanged();
    void floatingChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

  private:
    /// @brief Rewires the model change subscriptions and repaints.
    void subscribe(QAbstractListModel*& slot, QAbstractListModel* model);

    /// @brief Returns the geometry of every node keyed by operator id.
    QHash<quint64, NodeGeometry> nodeGeometries() const;

    /// @brief Resolves each link row into the two port points its curve spans.
    std::vector<Link> collectLinks() const;

    QAbstractListModel* nodes_ = nullptr;
    QAbstractListModel* links_ = nullptr;
    QColor linkColor_{"#3a3a46"};
    qreal nodeWidth_ = 80;
    qreal nodeHeight_ = 25;
    bool floatingActive_ = false;
    QPointF floatingOutput_;
    QPointF floatingInput_;
};

} // namespace enzo::ui
