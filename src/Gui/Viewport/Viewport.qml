import QtQuick
import Enzo

// Viewport panel hosting the graphics device surface.
Item {
    id: root

    ViewportItem {
        id: surface

        anchors.fill: parent
        backgroundColor: Theme.viewport.backgroundColor
    }
}
