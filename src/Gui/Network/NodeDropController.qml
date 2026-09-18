import QtQuick

// Tracks what releasing the current node drag would do, either pulling the
// selected nodes out of the graph or dropping the dragged node into the link
// it covers.
QtObject {
    id: root

    // The link layer the dragged node is hit tested against.
    property var links

    // How near the dragged node's center must fall to a link to drop into it.
    property real insertRadius: 20

    // True while shift is held, which turns a node drag into a bypass.
    property bool bypassHeld: false

    // The node a drag is moving and where its center sits.
    property var draggedNodeId: undefined
    property point draggedPoint

    // The link the dragged node covers, -1 when it covers none or a bypass drag
    // is pulling the selection out of the graph instead.
    readonly property int hoveredLink: draggedNodeId === undefined || bypassHeld ? -1 : links.linkAt(draggedPoint, insertRadius).linkIndex

    // What releasing the drag would rewire, null while no drag runs. It holds the
    // link indices the drop cuts and the links it wires in their place.
    readonly property var preview: draggedNodeId === undefined ? null : network.getDropPreview(draggedNodeId, hoveredLink, bypassHeld)

    // Moves the drag to a node and the canvas point its center sits on.
    function update(nodeId, canvasPoint) {
        draggedNodeId = nodeId;
        draggedPoint = canvasPoint;
    }

    // Ends the drag without applying it.
    function clear() {
        draggedNodeId = undefined;
    }

    // Applies the previewed drop, cutting the links it covers and wiring the ones
    // that replace them.
    function commit() {
        const applied = preview;
        clear();
        if (applied)
            network.applyDropPreview(applied);
    }
}
