import QtQuick
import Enzo

// Left aligned switch, on when the value is non zero.
Item {
    id: root

    required property var item
    implicitHeight: 22

    Rectangle {
        id: switchTrack

        width: 38
        height: 20
        radius: 10
        anchors.verticalCenter: parent.verticalCenter
        color: (root.item && root.item.value) ? Theme.var.accent : Theme.parameter.backgroundColor
        border.color: Theme.parameter.lineColor

        Rectangle {
            width: 16
            height: 16
            radius: 8
            anchors.verticalCenter: parent.verticalCenter
            x: (root.item && root.item.value) ? parent.width - width - 2 : 2
            color: Theme.var.text
            Behavior on x { NumberAnimation { duration: 90 } }
        }

        TapHandler {
            onTapped: root.item.value = root.item.value ? 0 : 1
        }
    }
}
