import QtQuick

// Tracks the link being dragged from a port to a target port. A link always runs
// from a source output down to a target input. While dragging, one end is the
// grabbed port and the other follows the cursor, depending on which side the drag
// began. There is no link in progress while linking is false.
QtObject {
    id: controller

    // The link layer queried for the port under a dropped link.
    property var layer
    // The view-model that wires a finished link into the graph.
    property var viewModel

    property bool linking: false
    property var originOpId
    property int originSlot: 0
    property bool fromOutput: true
    property point outputPoint
    property point inputPoint

    // The port the loose end is currently snapped to, undefined when it is free.
    property var hoverOpId
    property int hoverSlot: 0

    // Moves the loose end of the in-progress link to a canvas point.
    function setLooseEnd(canvasPoint) {
        if (fromOutput)
            inputPoint = canvasPoint;
        else
            outputPoint = canvasPoint;
    }

    // Starts a link from a grabbed port, anchoring its output or input end there.
    function begin(opId, slot, isOutput, canvasPoint) {
        originOpId = opId;
        originSlot = slot;
        fromOutput = isOutput;
        outputPoint = canvasPoint;
        inputPoint = canvasPoint;
        hoverOpId = undefined;
        linking = true;
    }

    // Tracks the cursor, snapping the loose end onto a port on another node.
    function update(canvasPoint) {
        // An output drag looks for an input to feed, an input drag for an output.
        const hit = layer.portAt(canvasPoint, !fromOutput);
        if (hit.opId !== undefined && hit.opId !== originOpId) {
            hoverOpId = hit.opId;
            hoverSlot = hit.slot;
            setLooseEnd(Qt.point(hit.x, hit.y));
        } else {
            hoverOpId = undefined;
            setLooseEnd(canvasPoint);
        }
    }

    // Releases the link, wiring it into the port the loose end is snapped to.
    function finish() {
        if (!linking)
            return;

        if (hoverOpId !== undefined) {
            if (fromOutput)
                viewModel.connectNodes(originOpId, originSlot, hoverOpId, hoverSlot);
            else
                viewModel.connectNodes(hoverOpId, hoverSlot, originOpId, originSlot);
        }
        hoverOpId = undefined;
        linking = false;
    }
}
