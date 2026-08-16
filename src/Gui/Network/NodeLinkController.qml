import QtQuick

// Tracks the link being drawn from a source output to a target input. One end
// stays at the grabbed port, the other follows the cursor. A drag commits on
// release, a click leaves the link trailing the cursor until a second click.
QtObject {
    id: controller

    // The view-model that wires a finished link into the graph and holds the node
    // model the loose end snaps against.
    property var viewModel

    property bool linking: false

    // True while a drag holds the loose end, false while it trails clicks.
    property bool dragging: false

    property var originNodeId
    property int originSlot: 0
    property bool fromOutput: true
    property point outputPoint
    property point inputPoint

    // The port the loose end is currently snapped to, undefined when it is free.
    property var hoverNodeId
    property int hoverSlot: 0

    // Moves the loose end of the in-progress link to a canvas point.
    function setLooseEnd(canvasPoint) {
        if (fromOutput)
            inputPoint = canvasPoint;
        else
            outputPoint = canvasPoint;
    }

    // Grabs a port. With no link in progress this anchors a fresh link there,
    // otherwise it places the in-progress link onto the port.
    function grab(nodeId, slot, isOutput, canvasPoint) {
        if (linking) {
            update(canvasPoint);
            finish();
            return;
        }
        originNodeId = nodeId;
        originSlot = slot;
        fromOutput = isOutput;
        outputPoint = canvasPoint;
        inputPoint = canvasPoint;
        hoverNodeId = undefined;
        linking = true;
        dragging = false;
    }

    // Drags the loose end, marking the link as held so it commits on release.
    function drag(canvasPoint) {
        dragging = true;
        update(canvasPoint);
    }

    // Ends a press. A drag commits the link, a click leaves it trailing.
    function release() {
        if (dragging)
            finish();
    }

    // Tracks the cursor, snapping the loose end onto a port on another node.
    function update(canvasPoint) {
        // An output drag looks for an input to feed, an input drag for an output.
        const hit = viewModel.nodes.getSnapPort(canvasPoint, !fromOutput);
        if (hit.nodeId !== undefined && hit.nodeId !== originNodeId) {
            hoverNodeId = hit.nodeId;
            hoverSlot = hit.slot;
            setLooseEnd(Qt.point(hit.x, hit.y));
        } else {
            hoverNodeId = undefined;
            setLooseEnd(canvasPoint);
        }
    }

    // Releases the link, wiring it into the port the loose end is snapped to.
    function finish() {
        if (!linking)
            return;

        if (hoverNodeId !== undefined) {
            if (fromOutput)
                viewModel.connectNodes(originNodeId, originSlot, hoverNodeId, hoverSlot);
            else
                viewModel.connectNodes(hoverNodeId, hoverSlot, originNodeId, originSlot);
        }
        hoverNodeId = undefined;
        linking = false;
        dragging = false;
    }

    // Abandons the in-progress link without wiring anything.
    function cancel() {
        hoverNodeId = undefined;
        linking = false;
        dragging = false;
    }
}
