#include "Gui/Network/NodeLinkLayer.h"

#include <QHash>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

namespace enzo::ui {

namespace {

// Stroke widths for a normal link, a hovered cut target, a rewire pickup preview,
// and a link a node drop would wire.
constexpr float kLinkWidth = 1;
constexpr float kCutWidth = 1;
constexpr float kRedirectWidth = 2;
constexpr float kPreviewWidth = 2;

// How long a cut link takes to dissolve, and how soft the dissolving edge is.
constexpr qreal kFadeMs = 150;
constexpr qreal kFadeSoftness = 0.2;

// How far a link's rounded corners reach from their corner point, and how many
// segments approximate each one's arc.
constexpr qreal kCornerRadius = 24;
constexpr int kCornerSegments = 8;

// How far the link dips straight down out of the output before turning sideways,
// and how tight that first turn is.
constexpr qreal kStubLength = 14;
constexpr qreal kStubRadius = 10;

// How round the two joints are where a link's stubs meet directly on a diagonal.
constexpr qreal kDiagonalRadius = 40;

// Below this much horizontal distance between output and input, the link renders as a straight
// line.
constexpr qreal kMinElbowHorizontalLength = 2 * (kStubRadius + kCornerRadius);

// Below this much distance between output and input, the link renders as a single straight
// segment with no stubs at all.
constexpr qreal kMinStubDistance = 2 * kStubLength;

/// @brief Returns the role number a model exposes under @p name, or -1 when absent.
int findRole(const QHash<int, QByteArray>& roles, const QByteArray& name)
{
    for (auto role = roles.cbegin(); role != roles.cend(); ++role)
        if (role.value() == name) return role.key();
    return -1;
}

/// @brief Appends the rounded turn at a corner where travel switches from @p directionIn
/// to @p directionOut.
///
/// @param maxTangentLength Cap on how far the arc reaches along either straight segment.
/// @note Leaves out the corner point itself. The caller supplies the straight segments
/// on either side, connecting to the point before and the point after this call.
void appendCorner(
    std::vector<QPointF>& points,
    const QPointF& corner,
    const QPointF& directionIn,
    const QPointF& directionOut,
    qreal radius,
    qreal maxTangentLength
)
{
    const QPointF towardBend = -directionIn;
    const qreal turnAngle = std::atan2(
        std::abs(towardBend.x() * directionOut.y() - towardBend.y() * directionOut.x()),
        towardBend.x() * directionOut.x() + towardBend.y() * directionOut.y()
    );

    if (radius <= 0 || turnAngle <= 0)
    {
        points.push_back(corner);
        return;
    }

    const qreal tangentLength = std::min(radius / std::tan(turnAngle / 2), maxTangentLength);
    const qreal arcRadius = tangentLength * std::tan(turnAngle / 2);

    const QPointF arcStart = corner - directionIn * tangentLength;
    const QPointF arcEnd = corner + directionOut * tangentLength;
    const QPointF bisector = towardBend + directionOut;
    const qreal bisectorLength = std::hypot(bisector.x(), bisector.y());
    const QPointF center =
        corner + bisector * (arcRadius / (std::sin(turnAngle / 2) * bisectorLength));

    const qreal startAngle = std::atan2(arcStart.y() - center.y(), arcStart.x() - center.x());
    const qreal endAngle = std::atan2(arcEnd.y() - center.y(), arcEnd.x() - center.x());
    qreal sweep = endAngle - startAngle;
    if (sweep > std::numbers::pi)
        sweep -= 2 * std::numbers::pi;
    else if (sweep < -std::numbers::pi)
        sweep += 2 * std::numbers::pi;

    points.push_back(arcStart);
    for (int segment = 1; segment <= kCornerSegments; ++segment)
    {
        const qreal angle = startAngle + sweep * static_cast<qreal>(segment) / kCornerSegments;
        points.push_back(center + QPointF(std::cos(angle), std::sin(angle)) * arcRadius);
    }
}

/// @brief Returns the points sampled along a route through a sequence of waypoints,
/// rounding every interior waypoint by its paired radius in @p radii.
///
/// @note A corner's reach along a neighboring leg is capped at half that leg's length
/// when the leg's other end is itself a rounded corner, and at the leg's full length
/// when that end is the route's start or end.
std::vector<QPointF>
roundedPath(const std::vector<QPointF>& waypoints, const std::vector<qreal>& radii)
{
    std::vector<QPointF> points;
    points.push_back(waypoints.front());
    for (std::size_t cornerIndex = 1; cornerIndex + 1 < waypoints.size(); ++cornerIndex)
    {
        const QPointF legIn = waypoints[cornerIndex] - waypoints[cornerIndex - 1];
        const QPointF legOut = waypoints[cornerIndex + 1] - waypoints[cornerIndex];
        const qreal legInLength = std::hypot(legIn.x(), legIn.y());
        const qreal legOutLength = std::hypot(legOut.x(), legOut.y());
        const QPointF directionIn = legInLength > 0 ? legIn / legInLength : QPointF(0, 1);
        const QPointF directionOut = legOutLength > 0 ? legOut / legOutLength : QPointF(0, 1);

        const bool legInShared = cornerIndex > 1;
        const bool legOutShared = cornerIndex + 2 < waypoints.size();
        const qreal maxTangentLength =
            std::min(legInLength / (legInShared ? 2 : 1), legOutLength / (legOutShared ? 2 : 1));

        appendCorner(
            points,
            waypoints[cornerIndex],
            directionIn,
            directionOut,
            radii[cornerIndex - 1],
            maxTangentLength
        );
    }
    points.push_back(waypoints.back());
    return points;
}

/// @brief Returns the points sampled along one link's route from output to input.
///
/// The link dips a stub straight down out of the output and arrives through a matching
/// stub straight down into the input, joined either directly or through a horizontal run.
///
/// @note Below @ref kMinElbowHorizontalLength of sideways distance, the two stubs run
/// straight into each other on a diagonal instead of through a horizontal run.
/// @note When the input sits above the output, the horizontal run detours upward
/// halfway across so the final stub still points down into the input.
/// @note When the output and input line up vertically, or sit within @ref kMinStubDistance
/// of each other, the link is a single straight segment with no stubs.
std::vector<QPointF> sampleLinkPath(const NodeLinkLayer::Link& link)
{
    const qreal horizontalLength = std::abs(link.input.x() - link.output.x());
    const qreal totalDistance = std::hypot(horizontalLength, link.input.y() - link.output.y());
    if (horizontalLength <= 0 || totalDistance < kMinStubDistance)
        return {link.output, link.input};

    const QPointF down(0, 1.0);
    const QPointF up(0, -1.0);
    const QPointF sideways(link.input.x() >= link.output.x() ? 1.0 : -1.0, 0);

    if (horizontalLength < kMinElbowHorizontalLength)
    {
        const QPointF outputStubEnd = link.output + down * kStubLength;
        const QPointF inputStubStart = link.input - down * kStubLength;
        return roundedPath(
            {link.output, outputStubEnd, inputStubStart, link.input},
            {kDiagonalRadius, kDiagonalRadius}
        );
    }

    if (link.input.y() >= link.output.y())
    {
        const qreal stubLength = std::min(kStubLength, link.input.y() - link.output.y());
        const QPointF stubEnd = link.output + down * stubLength;
        const QPointF corner(link.input.x(), stubEnd.y());
        return roundedPath(
            {link.output, stubEnd, corner, link.input},
            {kStubRadius, kCornerRadius}
        );
    }

    const qreal halfHorizontalLength = horizontalLength / 2;
    const QPointF outputStubEnd = link.output + down * kStubLength;
    const QPointF inputStubStart = link.input + up * kStubLength;
    const QPointF riseStart(
        link.output.x() + sideways.x() * halfHorizontalLength,
        outputStubEnd.y()
    );
    const QPointF riseEnd(riseStart.x(), inputStubStart.y());
    return roundedPath(
        {link.output, outputStubEnd, riseStart, riseEnd, inputStubStart, link.input},
        {kStubRadius, kCornerRadius, kCornerRadius, kStubRadius}
    );
}

/// @brief Returns the distance from a point to the nearest position on one segment.
qreal distanceToSegment(
    const QPointF& point,
    const QPointF& segmentStart,
    const QPointF& segmentEnd
)
{
    const QPointF along = segmentEnd - segmentStart;
    const qreal lengthSquared = along.x() * along.x() + along.y() * along.y();
    qreal t = 0;
    if (lengthSquared > 0)
    {
        const QPointF toPoint = point - segmentStart;
        t = std::clamp(
            (toPoint.x() * along.x() + toPoint.y() * along.y()) / lengthSquared,
            0.0,
            1.0
        );
    }
    const QPointF offset = point - (segmentStart + t * along);
    return std::hypot(offset.x(), offset.y());
}

/// @brief Returns which side of line @p lineStart to @p lineEnd the point @p point lies on.
qreal orientation(const QPointF& lineStart, const QPointF& lineEnd, const QPointF& point)
{
    return (lineEnd.x() - lineStart.x()) * (point.y() - lineStart.y()) -
           (lineEnd.y() - lineStart.y()) * (point.x() - lineStart.x());
}

/// @brief Whether segment @p firstStart to @p firstEnd crosses segment @p secondStart to @p
/// secondEnd.
bool segmentsIntersect(
    const QPointF& firstStart,
    const QPointF& firstEnd,
    const QPointF& secondStart,
    const QPointF& secondEnd
)
{
    const qreal firstStartSide = orientation(secondStart, secondEnd, firstStart);
    const qreal firstEndSide = orientation(secondStart, secondEnd, firstEnd);
    const qreal secondStartSide = orientation(firstStart, firstEnd, secondStart);
    const qreal secondEndSide = orientation(firstStart, firstEnd, secondEnd);
    return ((firstStartSide > 0) != (firstEndSide > 0)) &&
           ((secondStartSide > 0) != (secondEndSide > 0));
}

/// @brief Writes a polyline stroke with a color and width into an existing geometry node.
///
/// @note A fading link passes its cut point and a progress in [0, 1] so the vertex
/// alpha dissolves outward from the cut. A negative progress skips the dissolve.
void updateLinkNode(
    QSGGeometryNode* node,
    const std::vector<QPointF>& points,
    const QColor& color,
    float width,
    const QPointF& cutPoint,
    qreal progress
)
{
    // Distance of each sampled point from the cut, with the far end normalized to one.
    std::vector<qreal> distances(points.size(), 0);
    qreal maxDistance = 1;
    if (progress >= 0)
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            const QPointF offset = points[i] - cutPoint;
            distances[i] = std::hypot(offset.x(), offset.y());
            maxDistance = std::max(maxDistance, distances[i]);
        }

    // The stroke is a triangle strip extruding each point sideways along its normal,
    // since the scene graph backends ignore line widths other than one.
    QSGGeometry* geometry = node->geometry();
    geometry->allocate(static_cast<int>(points.size()) * 2);
    QSGGeometry::ColoredPoint2D* vertices = geometry->vertexDataAsColoredPoint2D();
    int vertex = 0;
    const qreal halfWidth = width / 2;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        // The normal at a point is perpendicular to the chord between its neighbors.
        const QPointF ahead = points[std::min(i + 1, points.size() - 1)];
        const QPointF behind = points[i > 0 ? i - 1 : 0];
        const QPointF chord = ahead - behind;
        const qreal chordLength = std::hypot(chord.x(), chord.y());
        const QPointF normal = chordLength > 0
                                   ? QPointF(-chord.y() / chordLength, chord.x() / chordLength)
                                   : QPointF(1, 0);

        // Points nearest the cut clear first, the dissolve reaching the ends as progress grows.
        const qreal fade =
            progress < 0
                ? 1.0
                : std::clamp((distances[i] / maxDistance - progress) / kFadeSoftness, 0.0, 1.0);
        const qreal alpha = color.alphaF() * fade;
        // The vertex color material expects color premultiplied by alpha.
        const auto premultiply = [&](int channel) { return static_cast<uchar>(channel * alpha); };
        for (const qreal side : {-halfWidth, halfWidth})
            vertices[vertex++].set(
                points[i].x() + normal.x() * side,
                points[i].y() + normal.y() * side,
                premultiply(color.red()),
                premultiply(color.green()),
                premultiply(color.blue()),
                static_cast<uchar>(alpha * 255)
            );
    }
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);
}

/// @brief Returns an empty geometry node wired to draw one polyline stroke.
QSGGeometryNode* buildLinkNode()
{
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGVertexColorMaterial);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

} // namespace

NodeLinkLayer::NodeLinkLayer(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);

    // Advance every dissolving cut link, dropping the ones that have finished.
    fadeTimer_.setInterval(16);
    connect(&fadeTimer_, &QTimer::timeout, this, [this] {
        for (FadingLink& fade : fadingLinks_)
            fade.progress += fadeTimer_.interval() / kFadeMs;
        std::erase_if(fadingLinks_, [](const FadingLink& fade) { return fade.progress >= 1; });
        if (fadingLinks_.empty()) fadeTimer_.stop();
        update();
    });
}

NodeListModel* NodeLinkLayer::nodes() const { return nodes_; }

QAbstractListModel* NodeLinkLayer::links() const { return links_; }

void NodeLinkLayer::setNodes(NodeListModel* model)
{
    if (nodes_ == model) return;
    if (nodes_) nodes_->disconnect(this);

    nodes_ = model;
    if (model) connectForRepaint(model);

    update();
    Q_EMIT nodesChanged();
}

void NodeLinkLayer::setLinks(QAbstractListModel* model)
{
    if (links_ == model) return;
    if (links_) links_->disconnect(this);

    links_ = model;
    if (model) connectForRepaint(model);

    update();
    Q_EMIT linksChanged();
}

void NodeLinkLayer::setHover(int linkIndex, LinkHover kind, bool atOutputEnd)
{
    if (linkIndex < 0) kind = LinkHover::None;
    if (hoverLink_ == linkIndex && hoverKind_ == kind && hoverAtOutputEnd_ == atOutputEnd) return;
    hoverLink_ = linkIndex;
    hoverKind_ = kind;
    hoverAtOutputEnd_ = atOutputEnd;
    update();
}

bool NodeLinkLayer::floatingActive() const { return floatingActive_; }

void NodeLinkLayer::setFloatingActive(bool active)
{
    if (floatingActive_ == active) return;
    floatingActive_ = active;
    Q_EMIT floatingChanged();
    update();
}

QPointF NodeLinkLayer::floatingOutput() const { return floatingOutput_; }

void NodeLinkLayer::setFloatingOutput(QPointF point)
{
    if (floatingOutput_ == point) return;
    floatingOutput_ = point;
    Q_EMIT floatingChanged();
    update();
}

QPointF NodeLinkLayer::floatingInput() const { return floatingInput_; }

void NodeLinkLayer::setFloatingInput(QPointF point)
{
    if (floatingInput_ == point) return;
    floatingInput_ = point;
    Q_EMIT floatingChanged();
    update();
}

QVariantList NodeLinkLayer::previewCutLinks() const { return previewCutLinks_; }

void NodeLinkLayer::setPreviewCutLinks(const QVariantList& linkIndices)
{
    if (previewCutLinks_ == linkIndices) return;
    previewCutLinks_ = linkIndices;
    Q_EMIT previewChanged();
    update();
}

QVariantList NodeLinkLayer::previewLinks() const { return previewLinks_; }

void NodeLinkLayer::setPreviewLinks(const QVariantList& links)
{
    if (previewLinks_ == links) return;
    previewLinks_ = links;
    Q_EMIT previewChanged();
    update();
}

void NodeLinkLayer::connectForRepaint(QAbstractItemModel* model)
{
    // Any row or value change means the links must redraw, including the per-frame
    // x and y updates of a live node drag.
    auto repaint = [this] { update(); };
    connect(model, &QAbstractItemModel::dataChanged, this, repaint);
    connect(model, &QAbstractItemModel::rowsInserted, this, repaint);
    connect(model, &QAbstractItemModel::rowsRemoved, this, repaint);
    connect(model, &QAbstractItemModel::modelReset, this, repaint);
}

std::vector<NodeLinkLayer::Link> NodeLinkLayer::collectLinks() const
{
    std::vector<Link> links;
    if (!nodes_ || !links_) return links;

    const QHash<int, QByteArray> roles = links_->roleNames();
    const int sourceNodeRole = findRole(roles, "sourceNode");
    const int sourceOutputRole = findRole(roles, "sourceOutput");
    const int targetNodeRole = findRole(roles, "targetNode");
    const int targetInputRole = findRole(roles, "targetInput");
    if (sourceNodeRole < 0 || sourceOutputRole < 0 || targetNodeRole < 0 || targetInputRole < 0)
        return links;

    const int rows = links_->rowCount();
    for (int row = 0; row < rows; ++row)
    {
        const QModelIndex index = links_->index(row, 0);
        const quint64 sourceNode = links_->data(index, sourceNodeRole).toULongLong();
        const quint64 targetNode = links_->data(index, targetNodeRole).toULongLong();
        const int sourceOutput = links_->data(index, sourceOutputRole).toInt();
        const int targetInput = links_->data(index, targetInputRole).toInt();

        // The curve leaves the source's output and enters the target's input. A link
        // whose nodes are not both in the snapshot yet has no points to draw.
        const std::optional<QPointF> output =
            nodes_->getPortPosition(sourceNode, sourceOutput, true);
        const std::optional<QPointF> input =
            nodes_->getPortPosition(targetNode, targetInput, false);
        if (!output || !input) continue;

        links.push_back(Link{*output, *input, row});
    }
    return links;
}

QVariantMap NodeLinkLayer::linkAt(QPointF canvasPoint, qreal radius) const
{
    std::optional<Link> nearest;
    qreal nearestDistance = radius;
    for (const Link& link : collectLinks())
    {
        const std::vector<QPointF> points = sampleLinkPath(link);
        for (std::size_t i = 1; i < points.size(); ++i)
        {
            const qreal distance = distanceToSegment(canvasPoint, points[i - 1], points[i]);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = link;
            }
        }
    }
    if (!nearest) return {{"linkIndex", -1}};

    // The end nearer the point comes loose in a pickup while the other stays anchored.
    const QPointF toOutput = canvasPoint - nearest->output;
    const QPointF toInput = canvasPoint - nearest->input;
    const bool atOutputEnd =
        std::hypot(toOutput.x(), toOutput.y()) < std::hypot(toInput.x(), toInput.y());
    const QPointF anchored = atOutputEnd ? nearest->input : nearest->output;
    return {
        {"linkIndex", nearest->linkIndex},
        {"atOutputEnd", atOutputEnd},
        {"anchorX", anchored.x()},
        {"anchorY", anchored.y()},
    };
}

int NodeLinkLayer::linkCrossing(QPointF from, QPointF to) const
{
    for (const Link& link : collectLinks())
    {
        const std::vector<QPointF> points = sampleLinkPath(link);
        for (std::size_t i = 1; i < points.size(); ++i)
            if (segmentsIntersect(from, to, points[i - 1], points[i])) return link.linkIndex;
    }
    return -1;
}

void NodeLinkLayer::fadeLink(int linkIndex, QPointF cutPoint)
{
    for (const Link& link : collectLinks())
    {
        if (link.linkIndex != linkIndex) continue;
        fadingLinks_.push_back(FadingLink{link, cutPoint, 0});
        if (!fadeTimer_.isActive()) fadeTimer_.start();
        update();
        return;
    }
}

QSGNode* NodeLinkLayer::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    std::vector<Link> links = collectLinks();

    // The dragged link is drawn alongside the committed ones while a port drag runs.
    if (floatingActive_) links.push_back(Link{floatingOutput_, floatingInput_});

    QSGNode* root = oldNode ? oldNode : new QSGNode;

    // Every drawn link gets its own child node, reused from the last paint in order.
    QSGNode* child = root->firstChild();
    auto claimNode = [&]() -> QSGGeometryNode* {
        if (child)
        {
            auto* claimed = static_cast<QSGGeometryNode*>(child);
            child = child->nextSibling();
            return claimed;
        }
        auto* built = buildLinkNode();
        root->appendChildNode(built);
        return built;
    };

    // A link the drop would cut leaves the canvas while the preview stands.
    const auto isPreviewCut = [this](int linkIndex) {
        for (const QVariant& cut : previewCutLinks_)
            if (cut.toInt() == linkIndex) return true;
        return false;
    };

    for (const Link& link : links)
    {
        if (isPreviewCut(link.linkIndex)) continue;

        const bool cutHovered = hoverKind_ == LinkHover::Cut && link.linkIndex == hoverLink_;
        const QColor& color = cutHovered ? cutColor_ : linkColor_;
        const float width = cutHovered ? kCutWidth : kLinkWidth;
        updateLinkNode(claimNode(), sampleLinkPath(link), color, width, QPointF(), -1);

        // The half a press would pick up draws tinted over the hovered link.
        if (hoverKind_ == LinkHover::Redirect && link.linkIndex == hoverLink_)
        {
            const std::vector<QPointF> points = sampleLinkPath(link);
            const auto middle = points.begin() + points.size() / 2;
            const std::vector<QPointF> half = hoverAtOutputEnd_
                                                  ? std::vector<QPointF>(points.begin(), middle + 1)
                                                  : std::vector<QPointF>(middle, points.end());
            updateLinkNode(claimNode(), half, redirectColor_, kRedirectWidth, QPointF(), -1);
        }
    }

    // The links the drop would wire in place of the ones it cuts.
    for (const QVariant& entry : nodes_ ? previewLinks_ : QVariantList())
    {
        const QVariantMap fields = entry.toMap();
        const std::optional<QPointF> output = nodes_->getPortPosition(
            fields["sourceNode"].toULongLong(),
            fields["sourceOutput"].toInt(),
            true
        );
        const std::optional<QPointF> input = nodes_->getPortPosition(
            fields["targetNode"].toULongLong(),
            fields["targetInput"].toInt(),
            false
        );
        if (!output || !input) continue;

        updateLinkNode(
            claimNode(),
            sampleLinkPath(Link{*output, *input}),
            previewColor_,
            kPreviewWidth,
            QPointF(),
            -1
        );
    }

    // Dissolving cut links come last so they draw on top of the live ones.
    for (const FadingLink& fade : fadingLinks_)
        updateLinkNode(
            claimNode(),
            sampleLinkPath(fade.link),
            cutColor_,
            kLinkWidth,
            fade.cutPoint,
            fade.progress
        );

    // Drop the nodes left over when the drawn count shrinks.
    while (child)
    {
        QSGNode* next = child->nextSibling();
        root->removeChildNode(child);
        delete child;
        child = next;
    }

    return root;
}

} // namespace enzo::ui
