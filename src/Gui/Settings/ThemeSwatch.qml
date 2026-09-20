import QtQuick
import Enzo

// The three colours that stand for a theme, as one rounded strip.
Item {
    id: root

    property var colors: []

    readonly property real radius: 5

    implicitWidth: 26
    implicitHeight: 16

    Rectangle {
        anchors.left: parent.left
        width: Math.round(root.width * 0.4)
        height: root.height
        color: root.colors[0] ?? "transparent"
        topLeftRadius: root.radius
        bottomLeftRadius: root.radius
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.round(root.width * 0.3)
        height: root.height
        color: root.colors[1] ?? "transparent"
    }

    Rectangle {
        anchors.right: parent.right
        width: Math.round(root.width * 0.3)
        height: root.height
        color: root.colors[2] ?? "transparent"
        topRightRadius: root.radius
        bottomRightRadius: root.radius
    }

    // Outlines the strip, so a dark theme still reads against the field behind it.
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.16)
    }
}
