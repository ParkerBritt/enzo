import QtQuick
import Enzo

// Turns mouse events into controller verbs. A left press picks the tool that
// owns the drag until the release.
MouseArea {
    id: root

    // The pan and zoom the cursor is mapped through.
    property var viewTransform

    // The tools a left press can hand the drag to.
    property var linkDrag
    property var cutter
    property var redirect
    property var selection

    // The cursor position over the network.
    readonly property alias cursorX: root.mouseX
    readonly property alias cursorY: root.mouseY

    // The same position on the panned and zoomed canvas.
    readonly property point cursorPoint: Qt.point(viewTransform.toCanvasX(root.mouseX), viewTransform.toCanvasY(root.mouseY))

    anchors.fill: parent
    acceptedButtons: Qt.MiddleButton | Qt.LeftButton
    hoverEnabled: true

    cursorShape: draggingLink ? Qt.ClosedHandCursor : redirect.overLink ? Qt.PointingHandCursor : Qt.ArrowCursor

    // The tool the press handed the drag to, null while no tool holds it.
    property var activeTool: null

    // True while a left drag is pulling a link out of a grabbed port.
    readonly property bool draggingLink: activeTool === linkDrag

    // True when the press grabbed a port, so the matching release is not also
    // read as a click that would finish the link.
    property bool grabbedOnPress: false

    // The last view position a middle drag passed through.
    property real panLastX: 0
    property real panLastY: 0

    // Returns the canvas point under a mouse event.
    function canvasPointOf(mouse) {
        return Qt.point(viewTransform.toCanvasX(mouse.x), viewTransform.toCanvasY(mouse.y));
    }

    onPressed: mouse => {
        panLastX = mouse.x;
        panLastY = mouse.y;
        grabbedOnPress = false;
        selection.applied = false;
        if (mouse.button !== Qt.LeftButton)
            return;

        activeTool = null;
        const canvasPoint = canvasPointOf(mouse);

        if (cutter.held) {
            cutter.press(canvasPoint);
            activeTool = cutter;
            return;
        }

        // A press near a port grabs the closest one across every node, so the
        // nearest port always wins over the topmost.
        const port = network.nodes.getGrabPort(canvasPoint);
        if (port.nodeId !== undefined) {
            linkDrag.grab(port.nodeId, port.index, port.isOutput, Qt.point(port.x, port.y));
            grabbedOnPress = true;
            if (linkDrag.linking)
                activeTool = linkDrag;
            return;
        }

        if (linkDrag.linking)
            return;

        if (redirect.press(canvasPoint)) {
            grabbedOnPress = true;
            activeTool = linkDrag;
            return;
        }

        selection.press(canvasPoint, (mouse.modifiers & Qt.ShiftModifier) !== 0);
        activeTool = selection;
    }

    // A left click commits a snapped link, drops an unsnapped one, or clears the
    // selection. The press that grabbed a port does none of these, and a click on
    // a node body is consumed by the node.
    onClicked: mouse => {
        if (mouse.button !== Qt.LeftButton || grabbedOnPress || selection.applied)
            return;

        if (cutter.held) {
            cutter.click(canvasPointOf(mouse));
            return;
        }

        if (linkDrag.linking)
            linkDrag.finish();
        else
            network.clearSelection();
    }

    // Ends the press, also when a popup such as the tab menu takes the mouse away
    // before the button comes up.
    function endPress() {
        if (activeTool)
            activeTool.release();
        activeTool = null;
    }

    onReleased: endPress()
    onCanceled: endPress()

    onExited: redirect.clearHover()

    onPositionChanged: mouse => {
        const canvasPoint = canvasPointOf(mouse);

        // Hands the move to the tool the press picked. A click placed link has no
        // tool and still trails the cursor.
        if (activeTool)
            activeTool.drag(canvasPoint);
        else if (linkDrag.linking)
            linkDrag.update(canvasPoint);

        // The hover preview mirrors what a press at this point would do.
        if (cutter.held)
            cutter.hover(canvasPoint);
        else if (selection.sweeping || draggingLink || linkDrag.linking || network.nodes.isOverNodeOrPort(canvasPoint))
            redirect.clearHover();
        else
            redirect.hover(canvasPoint);

        if (!(mouse.buttons & Qt.MiddleButton))
            return;

        viewTransform.pan(mouse.x - panLastX, mouse.y - panLastY);
        panLastX = mouse.x;
        panLastY = mouse.y;
    }

    onWheel: wheel => viewTransform.zoomAt(wheel.x, wheel.y, Math.sign(wheel.angleDelta.y))
}
