import QtQuick
import Enzo

// Moves the viewport camera with mouse drags and the wheel.
MouseArea {
    id: root

    required property ViewportItem surface

    property real lastX: 0
    property real lastY: 0

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton

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
            root.surface.pan(dx, dy);
        else if (mouse.buttons & Qt.RightButton)
            root.surface.dolly(-dx * 0.05);
        else
            root.surface.orbit(dx, dy);
    }
    onWheel: wheel => root.surface.dolly(-wheel.angleDelta.y / 120)
}
