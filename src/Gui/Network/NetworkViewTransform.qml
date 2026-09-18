import QtQuick
import "../Utils.js" as Utils

// The pan and zoom of the network view.
QtObject {
    id: transform

    // How fast scrolling changes the zoom.
    property real zoomSpeed: 0.2
    property real zoomMax: 5
    property real zoomMin: 0.1

    property real zoom: 1

    // Where the canvas origin sits in the view.
    property real panX: 0
    property real panY: 0

    // Returns the canvas position under a view position.
    function toCanvasX(viewPosX) {
        return (viewPosX - panX) / zoom;
    }
    function toCanvasY(viewPosY) {
        return (viewPosY - panY) / zoom;
    }

    // Slides the canvas by a distance in view pixels.
    function pan(deltaX, deltaY) {
        panX += deltaX;
        panY += deltaY;
    }

    // Zooms one step in the given direction, keeping the canvas point under the
    // view position fixed. A positive direction zooms in.
    function zoomAt(viewPosX, viewPosY, direction) {
        const oldZoom = zoom;
        const newZoom = Utils.clamp(oldZoom * (1 + direction * zoomSpeed), zoomMin, zoomMax);
        const scaleFactor = newZoom / oldZoom;

        panX = viewPosX - scaleFactor * (viewPosX - panX);
        panY = viewPosY - scaleFactor * (viewPosY - panY);
        zoom = newZoom;
    }
}
