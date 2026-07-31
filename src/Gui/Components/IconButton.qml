import QtQuick
import Enzo

// Small square icon button with a hover tint.
Rectangle {
    id: root

    property alias name: icon.name
    signal clicked()

    width: 22
    height: 22
    radius: 5
    color: mouse.containsMouse ? Theme.var.borderSoft : "transparent"
    opacity: enabled ? 1 : 0.35

    Icon {
        id: icon

        anchors.centerIn: parent
        size: 14
        color: Theme.var.textLabel
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
