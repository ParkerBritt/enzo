import QtQuick
import Enzo
import "../Components"

// The display toggles, camera picker and display node path shown over the viewport.
Item {
    id: root

    required property ViewportItem surface
    required property var nodePath
    required property var cameraPaths

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 13
        width: toggles.width + 8
        height: toggles.height + 8
        radius: 11
        color: Theme.viewport.hudColor
        border.color: Theme.viewport.hudBorderColor

        Column {
            id: toggles

            anchors.centerIn: parent
            spacing: 3

            DisplayToggle {
                name: "box"
                tooltip: "Wireframe  W"
                checked: root.surface.wireframeVisible
                onClicked: root.surface.wireframeVisible = !root.surface.wireframeVisible
            }

            DisplayToggle {
                name: "circle-dot"
                tooltip: "Points"
                checked: root.surface.pointsVisible
                onClicked: root.surface.pointsVisible = !root.surface.pointsVisible
            }
        }
    }

    // Lists the orbit camera first, then each camera primitive by its path.
    Dropdown {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 13
        labels: ["Perspective"].concat(root.cameraPaths)
        currentIndex: root.cameraPaths.indexOf(root.surface.viewCameraPath) + 1
        tooltip: "Camera"
        onActivated: index => root.surface.viewCameraPath = index === 0 ? "" : root.cameraPaths[index - 1]
    }

    NodePathLabel {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 15
        anchors.bottomMargin: 16
        path: root.nodePath
    }

    component DisplayToggle: IconButton {
        property bool checked: false

        width: 32
        height: 32
        radius: 8
        iconSize: 15
        surfaceColor: checked ? Theme.viewport.toggleOnColor : "transparent"
        iconColor: checked ? Theme.var.text : Theme.var.textLabel
    }
}
