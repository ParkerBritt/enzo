import QtQuick
import "."

// The card for every node in the graph.
Repeater {
    id: root

    property real viewZoom: 1

    // The link being drawn between ports.
    property var linkDrag

    // The preview of what the current node drag would rewire.
    property var nodeDrop

    // The cursor position on the canvas.
    property point cursorPoint

    // The port shown highlighted, the one a press would act on.
    readonly property var highlightedPort: {
        if (root.linkDrag.linking) {
            if (root.linkDrag.hoverNodeId === undefined)
                return null;
            return {
                nodeId: root.linkDrag.hoverNodeId,
                index: root.linkDrag.hoverIndex,
                isOutput: !root.linkDrag.fromOutput
            };
        }
        const port = network.nodes.getGrabPort(root.cursorPoint);
        return port.nodeId === undefined ? null : port;
    }

    model: network.nodes

    delegate: Node {
        id: nodeDelegate
        viewZoom: root.viewZoom

        modelX: model.x
        modelY: model.y

        // True while this node anchors either end of the link being dragged.
        readonly property bool linkEndpoint: root.linkDrag.linking && (model.nodeId === root.linkDrag.originNodeId || model.nodeId === root.linkDrag.hoverNodeId)

        // An endpoint node rises above the floating layer, so the link tucks
        // under its ports while still drawing over the nodes it crosses.
        z: linkEndpoint ? 2 : 0
        nodeId: model.nodeId
        label: model.name
        selected: model.selected
        primary: model.primary
        display: model.display
        inputPortCount: model.inputPortCount
        outputPortCount: model.outputPortCount
        multiInput: model.multiInput
        linking: root.linkDrag.linking

        // The highlighted port when it is one of this node's own.
        readonly property var highlight: root.highlightedPort && root.highlightedPort.nodeId === model.nodeId ? root.highlightedPort : null
        highlightedInput: highlight && !highlight.isOutput ? highlight.index : -1
        highlightedOutput: highlight && highlight.isOutput ? highlight.index : -1

        property bool selectedAtPress: false

        onPressed: additive => {
            selectedAtPress = model.selected;
            if (!model.selected)
                network.selectNode(model.nodeId, additive);
            root.nodeDrop.update(model.nodeId, Qt.point(model.x, model.y));
        }
        onClicked: additive => {
            if (selectedAtPress)
                network.selectNode(model.nodeId, additive);
            root.nodeDrop.clear();
        }
        onDragMoved: (dx, dy) => {
            network.stageSelectionMove(dx, dy);
            root.nodeDrop.update(model.nodeId, Qt.point(model.x, model.y));
        }
        onDragReleased: {
            network.commitSelectionMove();
            root.nodeDrop.commit();
        }
        onDisplayToggled: network.setDisplayNode(model.nodeId)
    }
}
