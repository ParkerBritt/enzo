#include "Gui/Network/NetworkViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/UndoRedo/ChangeDisplayFlagCommand.h"
#include "Engine/UndoRedo/ChangePrimaryNodeCommand.h"
#include "Engine/UndoRedo/ChangeSelectionCommand.h"
#include "Engine/UndoRedo/UndoStack.h"

#include <QPointF>
#include <QRectF>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>

namespace enzo::ui {

namespace {
/// The vertical space left between a node and the one created below it.
constexpr qreal chainedNodeGap = 30;

/// @brief Returns the map form of a link that QML and the link layer read.
QVariantMap makeLink(
    nt::NodeId sourceNode,
    unsigned int sourceOutput,
    nt::NodeId targetNode,
    unsigned int targetInput
)
{
    return {
        {"sourceNode", static_cast<qulonglong>(sourceNode)},
        {"sourceOutput", static_cast<int>(sourceOutput)},
        {"targetNode", static_cast<qulonglong>(targetNode)},
        {"targetInput", static_cast<int>(targetInput)},
    };
}

/// @brief Returns a drop preview, the links it cuts and the links it wires.
QVariantMap makeDropPreview(QVariantList cutLinks, QVariantList newLinks)
{
    return {{"cutLinks", std::move(cutLinks)}, {"newLinks", std::move(newLinks)}};
}
} // namespace

NetworkViewModel::NetworkViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    nodeCreatedSubscription_ =
        network.nodeCreated.connect([this](nt::NodeId nodeId) { nodes_.addNode(nodeId); });

    nodeRemovedSubscription_ =
        network.nodeRemoved.connect([this](nt::NodeId nodeId) { nodes_.removeNode(nodeId); });

    networkClearedSubscription_ = network.networkCleared.connect([this]() {
        nodes_.clear();
        nodeLinks_.clear();
    });

    selectedNodesSubscription_ =
        network.selectedNodesChanged.connect([this](std::vector<nt::NodeId> selectedNodeIds) {
            nodes_.setSelection(selectedNodeIds);
        });

    primaryNodeSubscription_ =
        network.primaryNodeChanged.connect([this](std::optional<nt::NodeId> primaryId) {
            nodes_.setPrimary(primaryId);
        });

    displayNodeSubscription_ =
        network.displayNodeChanged.connect([this](std::optional<nt::NodeId> displayId) {
            nodes_.setDisplay(displayId);
        });

    nodePositionSubscription_ =
        network.nodePositionChanged.connect([this](nt::NodeId nodeId, Vector2 pos) {
            nodes_.setPosition(nodeId, pos.x(), pos.y());
        });

    nodeLinkCreatedSubscription_ =
        network.nodeLinkCreated.connect([this](nt::NodeLink nodeLink) {
            nodeLinks_.addNodeLink(nodeLink);
        });

    nodeLinkRemovedSubscription_ =
        network.nodeLinkRemoved.connect([this](nt::NodeLink nodeLink) {
            nodeLinks_.removeNodeLink(nodeLink);
        });

    // Catches any graph state that already exists before the subscriptions are live.
    nodes_.resetFromNetwork();
    nodeLinks_.resetFromNetwork();
}

QAbstractListModel* NetworkViewModel::nodes() { return &nodes_; }

QAbstractListModel* NetworkViewModel::nodeLinks() { return &nodeLinks_; }

qreal NetworkViewModel::getNodeWidth() const { return NodeListModel::nodeWidth; }

qreal NetworkViewModel::getNodeHeight() const { return NodeListModel::nodeHeight; }

QVariantList NetworkViewModel::getNodeTypes() const
{
    QVariantList list;
    for (const nt::NodeType& info : nt::NodeTypeTable::getData())
    {
        QVariantMap entry;
        entry["label"] = QString::fromStdString(info.displayName);
        entry["name"] = QString::fromStdString(info.getFullName());
        list.append(entry);
    }
    for (const nt::NodeAlias& alias : nt::NodeTypeTable::getNodeAliases())
    {
        QVariantMap entry;
        entry["label"] = QString::fromStdString(alias.getLabel());
        entry["name"] = QString::fromStdString(alias.getFullName());
        list.append(entry);
    }
    return list;
}

void NetworkViewModel::createNode(const QString& fullName, qreal x, qreal y)
{
    nt::nm().createNode(
        fullName.toStdString(),
        Path("/"),
        "",
        {static_cast<float>(x), static_cast<float>(y)}
    );
}

bool NetworkViewModel::chainNodeToPrimary(const QString& fullName)
{
    auto& network = nt::nm();

    const std::optional<nt::NodeId> primaryId = network.getPrimaryNode();
    if (!primaryId) return false;

    const nt::Node& primaryNode = network.getNode(*primaryId);
    const Vector2 primaryPosition = primaryNode.getPosition();
    const bool primaryHasOutput = primaryNode.getMaxOutputs() > 0;

    const float rowSpacing = static_cast<float>(NodeListModel::nodeHeight + chainedNodeGap);
    const Vector2 belowPosition = {primaryPosition.x(), primaryPosition.y() + rowSpacing};

    // Creating, wiring and selecting the node collapse into a single undo step.
    nt::UndoTransaction transaction(network.undoStack());

    const nt::NodeId createdId =
        network.createNode(fullName.toStdString(), Path("/"), "", belowPosition);

    if (primaryHasOutput && network.getNode(createdId).takesInput())
        network.connectNodes(*primaryId, 0, createdId, 0);

    selectNode(createdId, false);
    return true;
}

void NetworkViewModel::selectNodes(
    const std::vector<nt::NodeId>& selection,
    std::optional<nt::NodeId> primaryId
)
{
    auto& network = nt::nm();

    const std::vector<nt::NodeId> prevSelection = network.getSelectedNodes();
    const std::optional<nt::NodeId> prevPrimary = network.getPrimaryNode();

    nt::UndoTransaction transaction(network.undoStack());

    if (selection != prevSelection)
    {
        network.undoStack().push(
            std::make_unique<nt::ChangeSelectionCommand>(prevSelection, selection)
        );
        network.setSelectedNodes(selection);
    }

    if (primaryId && primaryId != prevPrimary)
    {
        network.undoStack().push(
            std::make_unique<nt::ChangePrimaryNodeCommand>(prevPrimary, *primaryId)
        );
        network.setPrimaryNode(*primaryId);
    }
}

void NetworkViewModel::selectNode(qulonglong nodeId, bool additive)
{
    std::vector<nt::NodeId> nextSelection;
    if (additive)
    {
        nextSelection = nt::nm().getSelectedNodes();
        const auto found = std::find(nextSelection.begin(), nextSelection.end(), nodeId);
        if (found != nextSelection.end())
            nextSelection.erase(found);
        else
            nextSelection.push_back(nodeId);
    }
    else
    {
        nextSelection = {nodeId};
    }

    selectNodes(nextSelection, nodeId);
}

void NetworkViewModel::selectNodesInRect(QRectF canvasRect, bool additive)
{
    auto& network = nt::nm();

    const std::vector<nt::NodeId> boxedIds = nodes_.getNodesInRect(canvasRect);

    std::vector<nt::NodeId> nextSelection =
        additive ? network.getSelectedNodes() : std::vector<nt::NodeId>{};
    for (nt::NodeId nodeId : boxedIds)
    {
        const auto found = std::find(nextSelection.begin(), nextSelection.end(), nodeId);
        if (found == nextSelection.end()) nextSelection.push_back(nodeId);
    }

    // The last node the box swept over leads the new selection.
    const std::optional<nt::NodeId> nextPrimary =
        boxedIds.empty() ? std::nullopt : std::optional<nt::NodeId>(boxedIds.back());

    selectNodes(nextSelection, nextPrimary);
}

void NetworkViewModel::stageSelectionMove(qreal dx, qreal dy)
{
    nodes_.moveSelectedBy(static_cast<float>(dx), static_cast<float>(dy));
}

void NetworkViewModel::commitSelectionMove()
{
    auto& network = nt::nm();

    std::vector<nt::NodeId> selected = network.getSelectedNodes();
    if (selected.empty()) return;

    nt::UndoTransaction transaction(network.undoStack());
    for (nt::NodeId nodeId : selected)
    {
        const QPointF position = nodes_.getPosition(nodeId);
        network.moveNode(
            nodeId,
            {static_cast<float>(position.x()), static_cast<float>(position.y())}
        );
    }
}

void NetworkViewModel::undo() { nt::nm().undoStack().undo(); }

void NetworkViewModel::redo() { nt::nm().undoStack().redo(); }

void NetworkViewModel::deleteSelected()
{
    auto& network = nt::nm();

    // A copy is taken because deleting a node mutates the live selection.
    std::vector<nt::NodeId> selected = network.getSelectedNodes();
    if (selected.empty()) return;

    nt::UndoTransaction transaction(network.undoStack());
    for (nt::NodeId nodeId : selected)
        network.deleteNode(nodeId);
}

void NetworkViewModel::connectNodes(
    qulonglong sourceNode,
    int sourceOutput,
    qulonglong targetNode,
    int targetInput
)
{
    // A node cannot feed itself.
    if (sourceNode == targetNode) return;

    // The engine pushes its own undo command and emits nodeLinkCreated, which
    // the node link model already listens for, so the link appears through that path.
    nt::nm().connectNodes(sourceNode, sourceOutput, targetNode, targetInput);
}

void NetworkViewModel::removeLink(int linkIndex)
{
    // The engine emits nodeLinkRemoved, which the node link model already listens for.
    if (auto nodeLink = nodeLinks_.nodeLinkAt(linkIndex)) nt::nm().disconnectNodes(*nodeLink);
}

QVariantMap
NetworkViewModel::getDropPreview(qulonglong nodeId, int hoveredLink, bool bypassing) const
{
    return bypassing ? getBypassPreview() : getInsertPreview(nodeId, hoveredLink);
}

QVariantMap NetworkViewModel::getInsertPreview(qulonglong nodeId, int hoveredLink) const
{
    const QVariantMap nothingToDo = makeDropPreview({}, {});

    const std::optional<nt::NodeLink> nodeLink = nodeLinks_.nodeLinkAt(hoveredLink);
    if (!nodeLink) return nothingToDo;

    auto& network = nt::nm();

    // Leaves a wider selection alone, since dropping many nodes into one link has no
    // single meaning.
    if (network.getSelectedNodes().size() > 1) return nothingToDo;

    if (nodeLink->sourceNode == nodeId || nodeLink->targetNode == nodeId) return nothingToDo;

    const nt::Node& node = network.getNode(nodeId);
    if (!node.takesInput() || node.getMaxOutputs() == 0) return nothingToDo;

    // Leaves a node that already has an input alone, since the drop would take it over.
    if (network.graph().getInputNodeLink(nodeId, 0)) return nothingToDo;

    return makeDropPreview(
        {hoveredLink},
        {makeLink(nodeLink->sourceNode, nodeLink->sourceOutput, nodeId, 0),
         makeLink(nodeId, 0, nodeLink->targetNode, nodeLink->targetInput)}
    );
}

QVariantMap NetworkViewModel::getBypassPreview() const
{
    auto& network = nt::nm();
    const std::vector<nt::NodeId> bypassed = network.getSelectedNodes();

    const auto isBypassed = [&](nt::NodeId nodeId) {
        return std::find(bypassed.begin(), bypassed.end(), nodeId) != bypassed.end();
    };

    // Cuts every link touching the selection.
    QVariantList cutLinks;
    for (int linkIndex = 0; linkIndex < nodeLinks_.rowCount(); ++linkIndex)
    {
        const std::optional<nt::NodeLink> nodeLink = nodeLinks_.nodeLinkAt(linkIndex);
        if (nodeLink &&
            (isBypassed(nodeLink->sourceNode) || isBypassed(nodeLink->targetNode)))
            cutLinks.append(linkIndex);
    }

    // Returns the first input above a node that is not itself being pulled out.
    const auto getFeed = [&](nt::NodeId nodeId) {
        std::optional<nt::NodeLink> feed = network.graph().getInputNodeLink(nodeId, 0);
        for (std::size_t step = 0; feed && isBypassed(feed->sourceNode); ++step)
        {
            if (step > bypassed.size()) return std::optional<nt::NodeLink>{};
            feed = network.graph().getInputNodeLink(feed->sourceNode, 0);
        }
        return feed;
    };

    QVariantList newLinks;
    for (nt::NodeId nodeId : bypassed)
    {
        const std::optional<nt::NodeLink> feed = getFeed(nodeId);
        if (!feed) continue;

        for (const nt::NodeLink& outgoing : network.graph().getOutputs(nodeId))
        {
            if (isBypassed(outgoing.targetNode)) continue;
            newLinks.append(makeLink(
                feed->sourceNode,
                feed->sourceOutput,
                outgoing.targetNode,
                outgoing.targetInput
            ));
        }
    }
    return makeDropPreview(cutLinks, newLinks);
}

void NetworkViewModel::applyDropPreview(const QVariantMap& preview)
{
    // Reads the node links off the model first, since cutting one shifts the index
    // of every link after it.
    std::vector<nt::NodeLink> cut;
    for (const QVariant& linkIndex : preview["cutLinks"].toList())
        if (auto nodeLink = nodeLinks_.nodeLinkAt(linkIndex.toInt())) cut.push_back(*nodeLink);

    const QVariantList newLinks = preview["newLinks"].toList();
    if (cut.empty() && newLinks.isEmpty()) return;

    auto& network = nt::nm();

    nt::UndoTransaction transaction(network.undoStack());
    for (const nt::NodeLink& nodeLink : cut)
        network.disconnectNodes(nodeLink);
    for (const QVariant& link : newLinks)
    {
        const QVariantMap fields = link.toMap();
        network.connectNodes(
            fields["sourceNode"].toULongLong(),
            fields["sourceOutput"].toUInt(),
            fields["targetNode"].toULongLong(),
            fields["targetInput"].toUInt()
        );
    }
}

QVariantMap NetworkViewModel::getLinkEndpoints(int linkIndex) const
{
    const std::optional<nt::NodeLink> nodeLink = nodeLinks_.nodeLinkAt(linkIndex);
    if (!nodeLink) return {};
    return {
        {"sourceNode", static_cast<qulonglong>(nodeLink->sourceNode)},
        {"sourceOutput", static_cast<int>(nodeLink->sourceOutput)},
        {"targetNode", static_cast<qulonglong>(nodeLink->targetNode)},
        {"targetInput", static_cast<int>(nodeLink->targetInput)},
    };
}

void NetworkViewModel::setDisplayNode(qulonglong nodeId)
{
    auto& network = nt::nm();

    std::optional<nt::NodeId> prev = network.getDisplayNode();
    if (prev == nodeId) return;

    network.undoStack().push(std::make_unique<nt::ChangeDisplayFlagCommand>(prev, nodeId));
    network.setDisplayNode(nodeId);
}

void NetworkViewModel::setDisplayNodeToPrimary()
{
    const std::optional<nt::NodeId> primaryId = nt::nm().getPrimaryNode();
    if (!primaryId) return;

    setDisplayNode(*primaryId);
}

void NetworkViewModel::clearSelection() { selectNodes({}, std::nullopt); }

} // namespace enzo::ui
