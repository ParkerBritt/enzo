import QtQuick
import Enzo

// A drag selection box for nodes. Its outline and corners scale back with the
// zoom so they keep the same weight on screen.
Rectangle {
    id: root

    property real viewZoom: 1

    property bool sweeping: false

    // Whether a press could still grow into a box.
    property bool waiting: false

    // Whether the release selected the boxed nodes, so the click that follows it
    // is left alone.
    property bool applied: false

    // Whether the boxed nodes join the selection instead of replacing it.
    property bool additive: false

    // The canvas points the box spans, the press anchor and the cursor.
    property point anchor
    property point cursor

    // Anchors the box at the press point.
    function press(canvasPoint, pressAdditive) {
        waiting = true;
        additive = pressAdditive;
        anchor = canvasPoint;
        cursor = canvasPoint;
    }

    // Follows the cursor.
    function drag(canvasPoint) {
        const dragged = Math.hypot(canvasPoint.x - anchor.x, canvasPoint.y - anchor.y) * viewZoom;
        if (waiting && dragged > Qt.styleHints.startDragDistance) {
            waiting = false;
            sweeping = true;
        }
        if (sweeping)
            cursor = canvasPoint;
    }

    // Selects the boxed nodes and ends the sweep.
    function release() {
        waiting = false;
        if (!sweeping)
            return;
        network.selectNodesInRect(Qt.rect(x, y, width, height), additive);
        sweeping = false;
        applied = true;
    }

    visible: sweeping

    // The anchor and the cursor put in order, so the box has a positive size
    // whichever way the drag runs.
    x: Math.min(anchor.x, cursor.x)
    y: Math.min(anchor.y, cursor.y)
    width: Math.abs(cursor.x - anchor.x)
    height: Math.abs(cursor.y - anchor.y)

    radius: Theme.network.selectionRadius / viewZoom
    color: Theme.network.selectionColor
    border.color: Theme.network.selectionBorderColor
    border.width: 1 / viewZoom
    antialiasing: true
}
