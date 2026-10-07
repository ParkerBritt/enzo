import QtQuick
import Enzo

// Viewport panel hosting the graphics device surface.
Item {
    id: root

    ViewportItem {
        id: surface

        anchors.fill: parent
        viewModel: viewport
        backgroundColor: Theme.viewport.backgroundColor
        geometryColor: Theme.viewport.geometryColor
    }

    Shortcut {
        sequence: "w"
        onActivated: surface.toggleWireframe()
    }

    // Orbits on left drag, pans on middle drag, and dollies on horizontal right drag or the wheel.
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
                surface.dolly(-dx * 0.05);
            else
                surface.orbit(dx, dy);
        }
        onWheel: wheel => surface.dolly(-wheel.angleDelta.y / 120)
    }
}
