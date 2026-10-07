import QtQuick
import Enzo

// Viewport panel hosting the graphics device surface.
Item {
    id: root

    ViewportItem {
        id: viewportSurface

        anchors.fill: parent
        viewModel: viewport
        backgroundColor: Theme.viewport.backgroundColor
        geometryColor: Theme.viewport.geometryColor
    }

    ViewportMouse {
        anchors.fill: parent
        surface: viewportSurface
    }

    ViewportHud {
        anchors.fill: parent
        surface: viewportSurface
        nodePath: viewport.nodePath
        cameraPaths: viewport.cameraPaths
    }

    Shortcut {
        sequence: "w"
        onActivated: viewportSurface.wireframeVisible = !viewportSurface.wireframeVisible
    }
}
