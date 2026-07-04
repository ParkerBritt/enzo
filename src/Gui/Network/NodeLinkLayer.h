#pragma once

#include "Gui/Network/NodeListModel.h"
#include <QAbstractListModel>
#include <QColor>
#include <QPointF>
#include <QQuickItem>
#include <QTimer>
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

    Q_PROPERTY(NodeListModel* nodes READ nodes WRITE setNodes NOTIFY nodesChanged)
    Q_PROPERTY(QAbstractListModel* links READ links WRITE setLinks NOTIFY linksChanged)
    Q_PROPERTY(QColor linkColor MEMBER linkColor_ NOTIFY linkColorChanged)

    // Colors for the cut and rewire pickup hover previews.
    Q_PROPERTY(QColor cutColor MEMBER cutColor_ NOTIFY cutColorChanged)
    Q_PROPERTY(QColor redirectColor MEMBER redirectColor_ NOTIFY redirectColorChanged)

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
    /// What hovering the link under the cursor previews.
    enum class LinkHover
    {
        None,
        Cut,
        Redirect
    };
    Q_ENUM(LinkHover)

    /// One node link as the two port points its curve spans and its index in the link model.
    struct Link
    {
        QPointF output;
        QPointF input;
        int linkIndex = -1;
    };

    explicit NodeLinkLayer(QQuickItem* parent = nullptr);

    /// @brief Returns the link whose curve passes within @p radius of a point.
    /// @return A {linkIndex, atOutputEnd, anchorX, anchorY} map, with a linkIndex
    /// of -1 when no link is within reach.
    Q_INVOKABLE QVariantMap linkAt(QPointF canvasPoint, qreal radius) const;

    /// @brief Returns the index of the link whose curve a drag between two points crosses, or -1.
    ///
    /// Guards against a fast drag skipping between frames so a flick across a link
    /// still catches it.
    Q_INVOKABLE int linkCrossing(QPointF from, QPointF to) const;

    /// @brief Starts a fade of the link at an index, dissolving outward from a cut point.
    Q_INVOKABLE void fadeLink(int linkIndex, QPointF cutPoint);

    /// @brief Sets which link the cursor hovers and what the hover previews.
    /// @note A redirect hover names the end a press would pick up via @p atOutputEnd.
    Q_INVOKABLE void setHover(int linkIndex, LinkHover kind, bool atOutputEnd = false);

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
    void cutColorChanged();
    void redirectColorChanged();
    void floatingChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

  private:
    /// @brief Connects a model's change signals to a repaint.
    void connectForRepaint(QAbstractItemModel* model);

    /// @brief Resolves each link row into the two port points its curve spans.
    std::vector<Link> collectLinks() const;

    /// A cut link kept briefly to animate its dissolve from the point it was cut.
    struct FadingLink
    {
        Link link;
        QPointF cutPoint;
        qreal progress = 0;
    };

    NodeListModel* nodes_ = nullptr;
    QAbstractListModel* links_ = nullptr;
    QColor linkColor_;
    QColor cutColor_;
    QColor redirectColor_;
    int hoverLink_ = -1;
    LinkHover hoverKind_ = LinkHover::None;
    bool hoverAtOutputEnd_ = false;
    bool floatingActive_ = false;
    QPointF floatingOutput_;
    QPointF floatingInput_;

    std::vector<FadingLink> fadingLinks_;
    QTimer fadeTimer_;
};

} // namespace enzo::ui
