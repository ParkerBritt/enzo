import QtQuick
import Enzo

// Viewport panel hosting the OpenGL surface.
Item {
    id: root

    ViewportItem {
        id: surface

        anchors.fill: parent
        viewModel: viewport
        backgroundColor: Theme.viewport.backgroundColor
        gradientCenter: Theme.viewport.gradientCenter
        gradientEdge: Theme.viewport.gradientEdge
        geometryColor: Theme.viewport.geometryColor
    }

    // Left drag orbits, middle drag pans, horizontal right drag and wheel dolly.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton

        property real lastX: 0
        property real lastY: 0

        onPressed: mouse => {
            lastX = mouse.x;
            lastY = mouse.y;
        }
        onPositionChanged: mouse => {
            const dx = mouse.x - lastX;
            const dy = mouse.y - lastY;
            lastX = mouse.x;
            lastY = mouse.y;

            if (mouse.buttons & Qt.MiddleButton)
                surface.pan(dx, dy);
            else if (mouse.buttons & Qt.RightButton)
                surface.zoom(-dx * 0.05);
            else
                surface.orbit(dx, dy);
        }
        onWheel: wheel => surface.zoom(-wheel.angleDelta.y / 120)
    }
}
