import QtQuick
import Enzo

// The link cutter, which the left button becomes while 'c' is held.
QtObject {
    id: root

    // The link layer the cutter hit tests against.
    property var links

    // How near the cursor must fall to a link to cut it.
    property real hitRadius: 20

    // True while 'c' is held.
    property bool held: false

    // True while a left drag is sweeping across links.
    property bool cutting: false

    // The last canvas point a sweeping drag passed through.
    property point lastPoint

    // Starts a sweep at a canvas point.
    function press(canvasPoint) {
        cutting = true;
        lastPoint = canvasPoint;
    }

    // Cuts every link between the last point of the sweep and this one.
    function drag(canvasPoint) {
        if (!cutting)
            return;
        cut(links.linkCrossing(lastPoint, canvasPoint), canvasPoint);
        lastPoint = canvasPoint;
    }

    // Ends the sweep.
    function release() {
        cutting = false;
    }

    // Cuts the link under the cursor.
    function click(canvasPoint) {
        cut(links.linkAt(canvasPoint, hitRadius).linkIndex, canvasPoint);
        links.setHover(-1, NodeLinkLayer.None);
    }

    // Marks the link a click would cut.
    function hover(canvasPoint) {
        links.setHover(links.linkAt(canvasPoint, hitRadius).linkIndex, NodeLinkLayer.Cut);
    }

    // Removes a link, dissolving it outward from the cut point.
    function cut(linkIndex, canvasPoint) {
        if (linkIndex < 0)
            return;
        links.fadeLink(linkIndex, canvasPoint);
        network.removeLink(linkIndex);
    }
}
