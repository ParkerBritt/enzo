import QtQuick
import Enzo

// The pick up of a committed link by its nearer end.
QtObject {
    id: root

    // The link layer the pick up hit tests against.
    property var links

    // The controller the detached end is handed to.
    property var linkDrag

    // How near the cursor must fall to a link to pick it up.
    property real hitRadius: 20

    property bool enabled: true

    // The link under the cursor, -1 when there is none.
    property int hoveredLink: -1

    // True while the cursor rests on a link a press would pick up.
    readonly property bool overLink: enabled && hoveredLink >= 0

    // Detaches the pressed end of the link under the cursor and hands it to the
    // link controller. Returns true when a link was picked up.
    function press(canvasPoint) {
        if (!enabled)
            return false;

        const hit = links.linkAt(canvasPoint, hitRadius);
        if (hit.linkIndex < 0)
            return false;

        const ends = network.getLinkEndpoints(hit.linkIndex);
        network.removeLink(hit.linkIndex);

        const anchor = Qt.point(hit.anchorX, hit.anchorY);
        if (hit.atOutputEnd)
            linkDrag.grab(ends.targetNode, ends.targetInput, false, anchor);
        else
            linkDrag.grab(ends.sourceNode, ends.sourceOutput, true, anchor);
        linkDrag.drag(canvasPoint);
        return true;
    }

    // Marks the link a press would pick up and the end it would detach.
    function hover(canvasPoint) {
        if (!enabled)
            return;
        const hit = links.linkAt(canvasPoint, hitRadius);
        hoveredLink = hit.linkIndex;
        links.setHover(hit.linkIndex, NodeLinkLayer.Redirect, hit.atOutputEnd === true);
    }

    // Clears the hover mark.
    function clearHover() {
        hoveredLink = -1;
        links.setHover(-1, NodeLinkLayer.None);
    }
}
